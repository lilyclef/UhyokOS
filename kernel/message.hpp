#pragma once

enum class LayerOperation {
  Move, MoveRelative, Draw
};

struct Message {
  enum Type {
    kInterruptXHCI,
    kTimerTimeout,
    kKeyPush,
    kLayer,
    kLayerFinish,
  } type;

  // Sender task id
  uint64_t src_task;

  // To use several kinds of data as Messae, use union
  union {
    struct {
      unsigned long timeout;
      int value;
    } timer;

    struct {
      uint8_t modifier;
      uint8_t keycode;
      char ascii;
    } keyboard;

    struct {
      LayerOperation op;
      unsigned int layer_id;
      int x, y;
    } layer;
  } arg;
};
