#include "interrupt.hpp"

#include "asmfunc.h"
#include "segment.hpp"
#include "timer.hpp"

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

namespace {
  std::deque<Message>* msg_queue;

  // Compiler inserts Context Save and Return by attribute
  __attribute__((interrupt))
  void IntHandlerXHCI(InterruptFrame* frame) {
    msg_queue->push_back(Message{Message::kInterruptXHCI});
    NotifyEndOfInterrupt();
  }

  __attribute__((interrupt))
  void IntHandlerLAPICTimer(InterruptFrame* frame) {
    LAPICTimerOnInterrupt();
    NotifyEndOfInterrupt();
  }
}

void InitializeInterrupt(std::deque<Message>* msg_queue) {
  ::msg_queue = msg_queue;

  SetIDTEntry(idt[InterruptVector::kXHCI],
              MakeIDTAttr(DescriptorType::kInterruptGate, 0),
              reinterpret_cast<uint64_t>(IntHandlerXHCI),
              kKernelCS);
  SetIDTEntry(idt[InterruptVector::kLAPICTimer],
              MakeIDTAttr(DescriptorType::kInterruptGate, 0),
              reinterpret_cast<uint64_t>(IntHandlerLAPICTimer),
              kKernelCS);
  LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));
}
