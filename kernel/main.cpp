#include <cstdint>
#include <cstddef>
#include <cstdio>

#include <numeric>
#include <vector>
#include <deque>
#include <limits>

#include "frame_buffer_config.hpp"
#include "memory_map.hpp"
#include "graphics.hpp"
#include "mouse.hpp"
#include "font.hpp"
#include "console.hpp"
#include "pci.hpp"
#include "logger.hpp"
#include "usb/xhci/xhci.hpp"
#include "interrupt.hpp"
#include "asmfunc.h"
#include "segment.hpp"
#include "paging.hpp"
#include "memory_manager.hpp"
#include "window.hpp"
#include "layer.hpp"
#include "message.hpp"
#include "timer.hpp"
#include "acpi.hpp"
#include "keyboard.hpp"

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

// [12.19]
std::shared_ptr<Window> text_window;
unsigned int text_window_layer_id;
void InitializeTextWindow() {
  const int win_w = 160;
  const int win_h = 52;

  text_window = std::make_shared<Window>(
      win_w, win_h, screen_config.pixel_format);
  DrawWindow(*text_window->Writer(), "Text Box Test");
  DrawTextbox(*text_window->Writer(), {4, 24}, {win_w - 8, win_h - 24 - 4});

  text_window_layer_id = layer_manager->NewLayer()
    .SetWindow(text_window)
    .SetDraggable(true)
    .Move({350, 200})
    .ID();
  layer_manager->UpDown(text_window_layer_id, std::numeric_limits<int>::max());
}

int text_window_index;

void DrawTextCursor(bool visible) {
  const auto color = visible ? kDesktopFGColor : ToColor(0xFFFBE9);
  const auto pos = Vector2D<int>{10 + 8*text_window_index, 24 + 7};
  FillRectangle(*text_window->Writer(), pos, {1, 15}, color);
}

void InputTextWindow(char c) {
  if (c == 0) {
    return;
  }

  auto pos = []() { return Vector2D<int>{8 + 8*text_window_index, 24 + 6}; };

  const int max_chars = (text_window->Width() - 16) / 8 - 1;
  if (c == '\b' && text_window_index > 0) {
    DrawTextCursor(false);
    --text_window_index;
    FillRectangle(*text_window->Writer(), pos(), {8, 16}, ToColor(0xFFFBE9));
    DrawTextCursor(true);
  } else if (c >= ' ' && text_window_index < max_chars) {
    DrawTextCursor(false);
    WriteAscii(*text_window->Writer(), pos(), c, kDesktopFGColor);
    ++text_window_index;
    DrawTextCursor(true);
  }
  layer_manager->Draw(text_window_layer_id);
}

std::deque<Message>* main_queue;

/*
  KernelMain()がブートローダから呼び出される
  エントリポイントと呼ぶ
*/
// [8.6]
alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(
  const FrameBufferConfig& frame_buffer_config_ref,
  const MemoryMap& memory_map_ref,
  const acpi::RSDP& acpi_table) {
  MemoryMap memory_map{memory_map_ref};
  InitializeGraphics(frame_buffer_config_ref);
  InitializeConsole();
  printk("Welcome to Uhyo world!\n");
  SetLogLevel(kWarn);
  InitializeSegmentation();
  InitializePaging();
  InitializeMemoryManager(memory_map);
  ::main_queue = new std::deque<Message>(32);
  InitializeInterrupt(main_queue);
  InitializePCI();
  usb::xhci::Initialize();
  InitializeLayer();
  InitializeMainWindow();
  InitializeTextWindow();
  InitializeMouse();
  layer_manager->Draw({{0, 0}, ScreenSize()});
  acpi::Initialize(acpi_table);
  InitializeLAPICTimer(*main_queue);

  InitializeKeyboard(*main_queue);

  const int kTextboxCursorTimer = 1;
  const int kTimer05Sec = static_cast<int>(kTimerFreq * 0.5);
  __asm__("cli");
  timer_manager->AddTimer(Timer{kTimer05Sec, kTextboxCursorTimer});
  __asm__("sti");
  bool textbox_cursor_visible = false;

  char counter_str[128];

  while(true) {
    __asm__("cli");
    const auto tick = timer_manager->CurrentTick();
    __asm__("sti");

    sprintf(counter_str, "%010lu", tick);
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
      __asm__("sti\n\thlt");
      continue;
    }
    Message msg = main_queue->front();
    main_queue->pop_front();
    __asm__("sti");

    switch (msg.type) {
    case Message::kInterruptXHCI:
      usb::xhci::ProcessEvents();
      break;
    case Message::kTimerTimeout:
      if (msg.arg.timer.value == kTextboxCursorTimer) {
        __asm__("cli");
        timer_manager->AddTimer(
          Timer{msg.arg.timer.timeout + kTimer05Sec, kTextboxCursorTimer}
        );
        __asm__("sti");
        textbox_cursor_visible = !textbox_cursor_visible;
        DrawTextCursor(textbox_cursor_visible);
        layer_manager->Draw(text_window_layer_id);
      }
      break;
    case Message::kKeyPush:
      InputTextWindow(msg.arg.keyboard.ascii);
      break;
    default:
      Log(kError, "Unknown message type: %d\n", msg.type);
    }
  }
}

extern "C" void __cxa_pure_virtual() {
  while (1) __asm__("hlt");
}
