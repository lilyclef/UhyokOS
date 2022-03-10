#pragma once

#include <array>
#include <cstdint>
#include "x86_descriptor.hpp"

// [7.4] UNION for attributes of interrupt descripter
union InterruptDescriptorAttribute {
  uint16_t data;
  // non-named struct
  struct {
    // [type] [field_name] : [bit-width]
    uint16_t interrupt_stack_table : 3; // Basically 0
    uint16_t : 5;
    DescriptorType type : 4; // 14:Interrupt Gate, 15: Trap Gate
    uint16_t : 1;
    uint16_t descriptor_privilege_level : 2; // DPL Basically 0
    uint16_t present : 1; // Valid descripter Always 1
  } __attribute__((packed)) bits;
} __attribute__((packed));

// [7.4] [img 7.3]
struct InterruptDescriptor {
  uint16_t offset_low;      // 16 bit
  uint16_t segment_selector;// indicate code segment for executing interrupt handler
  InterruptDescriptorAttribute attr;
  uint16_t offset_middle;   // +16bit -> 32bit
  uint32_t offset_high;     // +32bit -> 64bit
  uint32_t reserved;
} __attribute__((packed)); // Prevent complier insets padding between fields.

extern std::array<InterruptDescriptor, 256> idt;

constexpr InterruptDescriptorAttribute MakeIDTAttr(
    DescriptorType type,
    uint8_t descriptor_privilege_level,
    bool present = true,
    uint8_t interrupt_stack_table = 0) {
  InterruptDescriptorAttribute attr{};
  attr.bits.interrupt_stack_table = interrupt_stack_table;
  attr.bits.type = type;
  attr.bits.descriptor_privilege_level = descriptor_privilege_level;
  attr.bits.present = present;
  return attr;
}

void SetIDTEntry(InterruptDescriptor& desc,
                 InterruptDescriptorAttribute attr,
                 uint64_t offset,
                 uint16_t segment_selector);

class InterruptVector {
 public:
  enum Number {
    kXHCI = 0x40,
  };
};

struct InterruptFrame {
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
};

void NotifyEndOfInterrupt();