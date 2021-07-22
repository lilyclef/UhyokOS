#include "usb/xhci/xhci.hpp"

#include "usb/setupdata.hpp"
#include "usb/device.hpp"
#include "usb/descriptor.hpp"

int printk(const char* format, ...);

namespace {
  using namespace usb::xhci;

  Error RegisterCommandRing(Ring* ring, MemMapRegister<CRCR_Bitmap>* crcr) {
    CRCR_Bitmap value{};
    value.bits.ring_cycle_state = true;
    value.SetPointer(reinterpret_cast<uint64_t>(ring->Buffer()));
    crcr->Write(value);
    return MAKE_ERROR(Error::kSuccess);
  }

  template <class EventTRBType>
  EventTRBType* WaitEvent(Controller& xhc) {
    while (true) {
      while (!xhc.PrimaryEventRing()->HasFront());
      if (auto trb = TRBDynamicCast<EventTRBType>(
            xhc.PrimaryEventRing()->Front())) {
        return trb;
      }
      xhc.PrimaryEventRing()->Pop();
    }
  }

  template <class EventTRBType, class IssuerTRBType>
  struct EventIssuer {
    EventTRBType* event_trb;
    IssuerTRBType* issuer_trb;
  };

  template <class EventTRBType, class IssuerTRBType>
  EventIssuer<EventTRBType, IssuerTRBType> WaitEvent(Controller& xhc) {
    while (true) {
      auto event_trb = WaitEvent<EventTRBType>(xhc);
      if (auto issuer = TRBDynamicCast<IssuerTRBType>(event_trb->Pointer())) {
        return {event_trb, issuer};
      }
      xhc.PrimaryEventRing()->Pop();
    }
  }

  void ResetPort(Port& port, bool debug = false) {
    if (debug) {
      printk("Resetting port %d\n", port.Number());
    }
    port.Reset();

    if (debug) {
      printk("Waiting port %d to be enabled\n", port.Number());
    }
    while (!port.IsEnabled());
  }

  uint8_t EnableSlot(Controller& xhc, bool debug = false) {
    EnableSlotCommandTRB cmd{};
    xhc.CommandRing()->Push(cmd);
    xhc.DoorbellRegisterAt(0)->Ring(0);

    if (debug) {
      printk("Waiting reply for the enable slot command\n");
    }
    auto event =
      WaitEvent<CommandCompletionEventTRB, EnableSlotCommandTRB>(xhc);
    uint8_t slot_id = event.event_trb->bits.slot_id;

    if (debug) {
      printk("received a response of EnableSlotCommand\n");
    }
    xhc.PrimaryEventRing()->Pop();
    xhc.DeviceManager()->AllocDevice(slot_id, xhc.DoorbellRegisterAt(slot_id));

    return slot_id;
  }

  void InitializeSlotContext(SlotContext& ctx, Port& port) {
    ctx.bits.route_string = 0;
    ctx.bits.root_hub_port_num = port.Number();
    ctx.bits.context_entries = 1;
    ctx.bits.speed = port.Speed();
  }

  unsigned int DetermineMaxPacketSizeForControlPipe(unsigned int slot_speed) {
    switch (slot_speed) {
    case 4: // Super Speed
        return 512;
    case 3: // High Speed
        return 64;
    default:
        return 8;
    }
  }

  void InitializeEP0Context(EndpointContext& ctx,
                            Ring* transfer_ring,
                            unsigned int max_packet_size) {
    ctx.bits.ep_type = 4; // Control Endpoint. Bidirectional.
    ctx.bits.max_packet_size = max_packet_size;
    ctx.bits.max_burst_size = 0;
    ctx.SetTransferRingBuffer(transfer_ring->Buffer());
    ctx.bits.dequeue_cycle_state = 1;
    ctx.bits.interval = 0;
    ctx.bits.max_primary_streams = 0;
    ctx.bits.mult = 0;
    ctx.bits.error_count = 3;
  }

  Error AddressDevice(Controller& xhc, Port& port, uint8_t slot_id, bool debug = false) {
    Device* dev = xhc.DeviceManager()->FindBySlot(slot_id);
    if (dev == nullptr) {
      if (debug) printk("failed to get a device: slot = %d\n", slot_id);
      return MAKE_ERROR(Error::kInvalidSlotID);
    }

    const auto ep0_dci = DeviceContextIndex(0, false);
    auto slot_ctx = dev->InputContext()->EnableSlotContext();
    auto ep0_ctx = dev->InputContext()->EnableEndpoint(ep0_dci);

    InitializeSlotContext(*slot_ctx, port);

    InitializeEP0Context(
        *ep0_ctx, dev->AllocTransferRing(ep0_dci, 32),
        DetermineMaxPacketSizeForControlPipe(slot_ctx->bits.speed));

    xhc.DeviceManager()->LoadDCBAA(slot_id);

    AddressDeviceCommandTRB addr_dev_cmd{dev->InputContext(), slot_id};
    xhc.CommandRing()->Push(addr_dev_cmd);
    xhc.DoorbellRegisterAt(0)->Ring(0);

    auto addr_dev_cmd_event =
      WaitEvent<CommandCompletionEventTRB, AddressDeviceCommandTRB>(xhc);

    if (auto sid = addr_dev_cmd_event.event_trb->bits.slot_id; sid != slot_id) {
      if (debug) printk("Unexpected slot id: %u\n", sid);
      return MAKE_ERROR(Error::kInvalidSlotID);
    }
    xhc.PrimaryEventRing()->Pop();

    if (debug) {
      printk("Addressed a device: slot id %u, port %u, speed %u\n",
             slot_id, port.Number(), slot_ctx->bits.speed);
    }

    return MAKE_ERROR(Error::kSuccess);
  }

  WithError<size_t> GetDescriptorWait(Controller& xhc, usb::xhci::Device& dev,
                                      int desc_type, int desc_index,
                                      uint8_t* buf, size_t n) {
    if (auto err = GetDescriptor(dev, 0, desc_type, desc_index,
                                 buf, n, true)) {
      return {0, err};
    }

    auto event_trb = WaitEvent<TransferEventTRB>(xhc);
    const auto transfer_length = event_trb->bits.trb_transfer_length;
    xhc.PrimaryEventRing()->Pop();

    return {n - transfer_length, MAKE_ERROR(Error::kSuccess)};
  }
}

namespace usb::xhci {

  Controller::Controller(uintptr_t mmio_base)
      : mmio_base_{mmio_base},
        cap_{reinterpret_cast<CapabilityRegisters*>(mmio_base)},
        op_{reinterpret_cast<OperationalRegisters*>(
            mmio_base + cap_->CAPLENGTH.Read())},
        max_ports_{static_cast<uint8_t>(
            cap_->HCSPARAMS1.Read().bits.max_ports)} {
  }

  Error Controller::Initialize() {
    if (auto err = devmgr_.Initialize(kDeviceSize)) {
      return err;
    }

    // Host controller must be halted before resetting it.
    if (!op_->USBSTS.Read().bits.host_controller_halted) {
      auto usbcmd = op_->USBCMD.Read();
      usbcmd.bits.run_stop = false;  // stop
      op_->USBCMD.Write(usbcmd);
      while (!op_->USBSTS.Read().bits.host_controller_halted);
    }

    // Reset controller
    auto usbcmd = op_->USBCMD.Read();
    usbcmd.bits.host_controller_reset = true;
    op_->USBCMD.Write(usbcmd);
    while (op_->USBCMD.Read().bits.host_controller_reset);
    while (op_->USBSTS.Read().bits.controller_not_ready);

    //printk("MaxSlots: %u\n", cap_->HCSPARAMS1.Read().bits.max_device_slots);
    // Set "Max Slots Enabled" field in CONFIG.
    auto config = op_->CONFIG.Read();
    config.bits.max_device_slots_enabled = kDeviceSize;
    op_->CONFIG.Write(config);

    DCBAAP_Bitmap dcbaap{};
    dcbaap.SetPointer(reinterpret_cast<uint64_t>(devmgr_.DeviceContexts()));
    op_->DCBAAP.Write(dcbaap);

    auto primary_interrupter = &InterrupterRegisterSets()[0];
    if (auto err = cr_.Initialize(32)) {
        return err;
    }
    if (auto err = RegisterCommandRing(&cr_, &op_->CRCR)) {
        return err; }
    if (auto err = er_.Initialize(32, primary_interrupter)) {
        return err;
    }

    // Enable interrupt for the primary interrupter
    auto iman = primary_interrupter->IMAN.Read();
    iman.bits.interrupt_pending = true;
    iman.bits.interrupt_enable = true;
    primary_interrupter->IMAN.Write(iman);

    // Enable interrupt for the controller
    usbcmd = op_->USBCMD.Read();
    usbcmd.bits.interrupter_enable = true;
    op_->USBCMD.Write(usbcmd);

    return MAKE_ERROR(Error::kSuccess);
  }

  Error Controller::Run() {
    // Run the controller
    auto usbcmd = op_->USBCMD.Read();
    usbcmd.bits.run_stop = true;
    op_->USBCMD.Write(usbcmd);
    op_->USBCMD.Read();

    while (op_->USBSTS.Read().bits.host_controller_halted);

    return MAKE_ERROR(Error::kSuccess);
  }

  DoorbellRegister* Controller::DoorbellRegisterAt(uint8_t index) {
    return &DoorbellRegisters()[index];
  }

  Error ConfigurePort(Controller& xhc, Port& port) {
    if (!port.IsConnected()) {
      return MAKE_ERROR(Error::kPortNotConnected);
    }

    ResetPort(port);
    auto slot_id = EnableSlot(xhc);

    if (auto err = AddressDevice(xhc, port, slot_id, true)) {
      return err;
    }

    auto dev = xhc.DeviceManager()->FindBySlot(slot_id);
    if (dev == nullptr) {
      return MAKE_ERROR(Error::kInvalidSlotID);
    }

    if (auto err = dev->StartInitialize()) {
      return err;
    }

    auto event_trb = WaitEvent<TransferEventTRB>(xhc);
    dev->OnTransferEventReceived(*event_trb);
    xhc.PrimaryEventRing()->Pop();

    auto device_desc = DescriptorDynamicCast<DeviceDescriptor>(dev->Buffer());
    if (device_desc == nullptr) {
      return MAKE_ERROR(Error::kInvalidDescriptor);
    }

    uint8_t buf[256];

    printk("device descriptor (%lu bytes):", dev->Buffer()[0]);
    for (int i = 0; i < dev->Buffer()[0]; ++i) {
      printk(" %02x", dev->Buffer()[i]);
    }
    printk("\n");

    const auto num_configurations = device_desc->num_configurations;
    for (int config_index = 0; config_index < num_configurations; ++config_index) {
      printk("Getting Configuration Descriptor: index=%d\n", config_index);
      auto desc_length = GetDescriptorWait(xhc, *dev, descriptor_type::kConfiguration,
                                           config_index, buf, sizeof(buf));
      if (desc_length.error) {
        return desc_length.error;
      }
      auto config_desc = DescriptorDynamicCast<ConfigurationDescriptor>(buf);
      if (config_desc == nullptr) {
        return MAKE_ERROR(Error::kInvalidDescriptor);
      }
      if (desc_length.value < config_desc->total_length) {
        return MAKE_ERROR(Error::kBufferTooSmall);
      }

      uint8_t* p = buf + config_desc->length;
      const auto num_interfaces = config_desc->num_interfaces;
      for (int if_index = 0; if_index < num_interfaces; ++if_index) {
        auto if_desc = DescriptorDynamicCast<InterfaceDescriptor>(p);
        if (if_desc == nullptr) {
          return MAKE_ERROR(Error::kInvalidDescriptor);
        }
        printk("Interface Descriptor: class=%d, sub=%d, protocol=%d\n",
            if_desc->interface_class,
            if_desc->interface_sub_class,
            if_desc->interface_protocol);

        p += if_desc->length;
        const auto num_endpoints = if_desc->num_endpoints;
        int num_endpoints_found = 0;

        while (num_endpoints_found < num_endpoints) {
          if (auto if_desc = DescriptorDynamicCast<InterfaceDescriptor>(p)) {
            return MAKE_ERROR(Error::kInvalidDescriptor);
          }
          if (auto ep_desc = DescriptorDynamicCast<EndpointDescriptor>(p)) {
            printk("Endpoint Descriptor: addr=0x%02x, attr=0x%02x\n",
                ep_desc->endpoint_address,
                ep_desc->attributes);
            ++num_endpoints_found;
          } else if (auto hid_desc = DescriptorDynamicCast<HIDDescriptor>(p)) {
            printk("HID Descriptor: release=0x%02x, num_desc=%d",
                hid_desc->hid_release,
                hid_desc->num_descriptors);
            for (int i = 0; i < hid_desc->num_descriptors; ++i) {
              printk(", desc_type=%d, len=%d",
                  hid_desc->GetClassDescriptor(i)->descriptor_type,
                  hid_desc->GetClassDescriptor(i)->descriptor_length);
            }
            printk("\n");
          }
          p += p[0];
        }
      }
    }

        return MAKE_ERROR(Error::kSuccess);
  }
}
