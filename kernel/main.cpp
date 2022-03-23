#include <cstdint>
#include <cstddef>
#include <cstdio>

#include <numeric>
#include <vector>

#include "frame_buffer_config.hpp"
#include "memory_map.hpp"
#include "graphics.hpp"
#include "mouse.hpp"
#include "font.hpp"
#include "console.hpp"
#include "pci.hpp"
#include "logger.hpp"
#include "usb/memory.hpp"
#include "usb/device.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"
#include "interrupt.hpp"
#include "asmfunc.h"
#include "queue.hpp"
#include "segment.hpp"
#include "paging.hpp"
#include "memory_manager.hpp"
#include "window.hpp"
#include "layer.hpp"
#include "timer.hpp"
#include "message.hpp"



int printk(const char* format, ...) {
  va_list ap;
  int result;
  char s[1024];

  va_start(ap, format);
  result = vsprintf(s, format, ap);
  va_end(ap);

  console->PutString(s);
  return result;
}

char memory_manager_buf[sizeof(BitmapMemoryManager)];
BitmapMemoryManager* memory_manager;

// Mouse Lib
// [9.19] Define MouseObserver()
unsigned int mouse_layer_id;
Vector2D<int> screen_size;
Vector2D<int> mouse_position;


void MouseObserver(uint8_t buttons, int8_t displacement_x, int8_t displacement_y) {
  static unsigned int mouse_drag_layer_id = 0;
  static uint8_t previous_buttons = 0;

  const auto oldpos = mouse_position;
   auto newpos = mouse_position + Vector2D<int>{displacement_x, displacement_y};
   newpos = ElementMin(newpos, screen_size + Vector2D<int>{-1, -1});
   mouse_position = ElementMax(newpos, {0, 0});
 
  const auto posdiff = mouse_position - oldpos;

   layer_manager->Move(mouse_layer_id, mouse_position);

  const bool previous_left_pressed = (previous_buttons & 0x01);
  const bool left_pressed = (buttons & 0x01);
  if (!previous_left_pressed && left_pressed) {
    auto layer = layer_manager->FindLayerByPosition(mouse_position, mouse_layer_id);
    if (layer && layer->IsDraggable()) {
      mouse_drag_layer_id = layer->ID();
    }
  } else if (previous_left_pressed && left_pressed) {
    if (mouse_drag_layer_id > 0) {
      layer_manager->MoveRelative(mouse_drag_layer_id, posdiff);
    }
  } else if (previous_left_pressed && !left_pressed) {
    mouse_drag_layer_id = 0;
  }
  previous_buttons = buttons;
 }

std::shared_ptr<Window> main_window;
unsigned int main_window_layer_id;
void InitializeMainWindow() {
  main_window = std::make_shared<Window>(
      250, 100, screen_config.pixel_format);
  DrawWindow(*main_window->Writer(), "Application");
  WriteString(*main_window->Writer(), {24, 28}, "Ashitamo I-hini naruyone?", kDesktopFGColor);
  WriteString(*main_window->Writer(), {24, 44}, "Uhyo~(:3", kDesktopFGColor);


  main_window_layer_id = layer_manager->NewLayer()
    .SetWindow(main_window)
    .SetDraggable(true)
    .Move({300, 100})
    .ID();

  layer_manager->UpDown(main_window_layer_id, std::numeric_limits<int>::max());

}
// Mouse Lib End

// [7.1] Definition of Interrupt Handler for xHCI
usb::xhci::Controller* xhc;

std::deque<Message>* main_queue;

/*
  KernelMain()がブートローダから呼び出される
  エントリポイントと呼ぶ
*/
// [8.6]
alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(const FrameBufferConfig& frame_buffer_config_ref, const MemoryMap& memory_map_ref) {
  MemoryMap memory_map{memory_map_ref};
  InitializeGraphics(frame_buffer_config_ref);
  InitializeConsole();
  printk("Welcome to Uhyo world!\n");
  SetLogLevel(kWarn);

  SetupSegments();

  const uint16_t kernel_cs = 1 << 3;
  const uint16_t kernel_ss = 2 << 3;
  SetDSAll(0);
  SetCSSS(kernel_cs, kernel_ss);

  SetupIdentityPageTable();
  ::memory_manager = new(memory_manager_buf) BitmapMemoryManager;

  const auto memory_map_base = reinterpret_cast<uintptr_t>(memory_map.buffer);
   uintptr_t available_end = 0;
  printk("Welcome to Uhyo world!3\n");
  // メモリマップはメモリディスクリプタの配列->順番に表示
  for (uintptr_t iter = memory_map_base;
       iter < memory_map_base + memory_map.map_size;
       iter += memory_map.descriptor_size) {
    auto desc = reinterpret_cast<const MemoryDescriptor*>(iter);
    if (available_end < desc->physical_start) {
      memory_manager->MarkAllocated(
          FrameID{available_end / kBytesPerFrame},
          (desc->physical_start - available_end) / kBytesPerFrame);
    }

    const auto physical_end =
      desc->physical_start + desc->number_of_pages * kUEFIPageSize;
    if (IsAvailable(static_cast<MemoryType>(desc->type))) {
      available_end = physical_end;
    } else {
      printk("fst %ld", desc->physical_start / kBytesPerFrame);
      printk("snd %ld", desc->number_of_pages * kUEFIPageSize / kBytesPerFrame);
      memory_manager->
      MarkAllocated(
          FrameID{desc->physical_start / kBytesPerFrame},
          desc->number_of_pages * kUEFIPageSize / kBytesPerFrame);
    }
  }

  // [9.2]
  memory_manager->SetMemoryRange(FrameID{1}, FrameID{available_end / kBytesPerFrame});

  if (auto err = InitializeHeap(*memory_manager)) {
    Log(kError, "failed to allocate pages: %s at%s:%d\n", err.Name(), err.File(), err.Line());
    exit(1);
  }


  // [6.17] List PCI Devices
  auto err = pci::ScanAllBus();
  Log(kDebug, "ScanAllBus: %s\n", err.Name());

  for (int i = 0; i < pci::num_device; ++i) {
    const auto& dev = pci::devices[i];
    auto vendor_id = pci::ReadVendorId(dev);
    auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
    Log(kDebug, "%d.%d.%d: vend %04x, class %08x, head %02x\n",
        dev.bus, dev.device, dev.function,
        vendor_id, class_code, dev.header_type);
  }

  // [6.23] Setting for connected port by searching USB port
  usb::HIDMouseDriver::default_observer = MouseObserver;

  // [9.20] Generate 2 layers
  screen_size.x = screen_config.horizontal_resolution;
  screen_size.y = screen_config.vertical_resolution;

  auto mouse_window = std::make_shared<Window>(
      kMouseCursorWidth, kMouseCursorHeight, screen_config.pixel_format);
  mouse_window->SetTransparentColor(kMouseTransparentColor);
  DrawMouseCursor(mouse_window->Writer(), {0, 0});
  mouse_position = {200, 200};
  //InitializeSegmentation();
  //InitializePaging();
  //InitializeMemoryManager(memory_map);
  ::main_queue = new std::deque<Message>(32);

  InitializeInterrupt(main_queue);
  //InitializePCI();
  usb::xhci::Initialize();
  InitializeLayer();
  InitializeMainWindow();
  //InitializeMouse();


  mouse_layer_id = layer_manager->NewLayer()
    .SetWindow(mouse_window)
    .Move(mouse_position)
    .ID();
  layer_manager->UpDown(mouse_layer_id, 3);
  layer_manager->Draw({{0, 0}, ScreenSize()});

  // [10.9]
  char counter_str[128];
  unsigned int counter = 0;

  while(true) {
    // [10.10]
    ++counter;
    sprintf(counter_str, "%010u", counter);
    FillRectangle(*main_window->Writer(), {24, 66}, {8 * 10, 16}, {255, 251, 233});
    WriteString(*main_window->Writer(), {24, 66}, counter_str, kDesktopFGColor);
    layer_manager->Draw(main_window_layer_id);

    // cli: Clear Interrupt flag
    // Interrupt Flag of the CPU is set 0
    __asm__("cli");
    if (main_queue->size() == 0) {
      // sti: Set Interrupt flag
      // Interrupt Flag of the CPU is set 1
      // hlt : Stop CPU since a new interrupt comes
      __asm__("sti");
      continue;
    }
    Message msg = main_queue->front();
    main_queue->pop_front();
    __asm__("sti");

    switch (msg.type) {
    case Message::kInterruptXHCI:
      usb::xhci::ProcessEvents();
      break;
    default:
      Log(kError, "Unknown message type: %d\n", msg.type);
    }
  }
}

extern "C" void __cxa_pure_virtual() {
  while (1) __asm__("hlt");
}
