#include "interrupt.hpp"

// [7.3] Definition of Interrupt Descripter Table
std::array<InterruptDescriptor, 256> idt;

// [7.5] Implementation of SetIDTEntry
// x86 architecture has grown 16bits -> 32bits -> 64bits
void SetIDTEntry(InterruptDescriptor& desc,
                 InterruptDescriptorAttribute attr,
                 uint64_t offset,
                 uint16_t segment_selector) {
  desc.attr = attr;
  desc.offset_low = offset & 0xffffu;
  desc.offset_middle = (offset >> 16) & 0xffffu;
  desc.offset_high = offset >> 32;
  desc.segment_selector = segment_selector;
}

// [7.2] NotifyEndOfInterrupt
// Write 0 on 0xfee000b0 (End of Interrupt Register)
// CPU Registers are set 0xfee00000 - 0xfee00400 1024bytes on the memory
void NotifyEndOfInterrupt() {
  // volatile prevents c++ compiler removes setting 0 variable as unused variable.
  volatile auto end_of_interrupt = reinterpret_cast<uint32_t*>(0xfee000b0);
  *end_of_interrupt = 0;
}