#pragma once

struct Message {
  enum Type {
    kInterruptXHCI,
    kTimerTimeout,
    kKeyPush,
  } type;

  // To use several kinds of data as Messae, use union
  union {
    struct {
      unsigned long timeout;
      int value;
    } timer;

    struct {
      uint8_t keycode;
      char ascii;
    } keyboard;
  } arg;
};
