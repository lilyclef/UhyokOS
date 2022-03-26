#pragma once

struct Message {
  enum Type {
    kInterruptXHCI,
    kTimerTimeout,
  } type;

  // To use several kinds of data as Messae, use union
  union {
    struct {
      unsigned long timeout;
      int value;
    } timer;
  } arg;
};
