#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"
#include "console.hpp"
#include "pci.hpp"

// (:3 配置newを設定するための準備 begin
/*void* operator new(size_t size, void* buf) {
  return buf;
  }*/

void operator delete(void* obj) noexcept {
}
// (:3 配置newを設定するための準備 end

// (:3 グローバル変数 begin
const PixelColor kDesktopBGColor{181, 234, 234};
const PixelColor kDesktopFGColor{142, 127, 127};

const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
  "@              ",
  "@@             ",
  "@@@            ",
  "@@@@           ",
  "@@.@@          ",
  "@@..@@         ",
  "@@...@@        ",
  "@@....@@       ",
  "@@.....@@      ",
  "@@......@@     ",
  "@@.......@@    ",
  "@@........@@   ",
  "@@.........@@  ",
  "@@.......@@@@@ ",
  "@@......@@@@@@@",
  "@@......@      ",
  "@@..@@..@      ",
  "@@.@@ @..@     ",
  "@@@@   @..@    ",
  "@@@    @..@    ",
  "@@      @..@   ",
  "@       @..@   ",
  "         @..@  ",
  "         @@@@  ",
};

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;
char console_buf[sizeof(Console)];
Console* console;
// (:3 グローバル変数 end


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


/*
  KernelMain()がブートローダから呼び出される
  エントリポイントと呼ぶ
*/
extern "C" void KernelMain(const FrameBufferConfig& frame_buffer_config) {
  switch (frame_buffer_config.pixel_format) {
  case kPixelRGBResv8BitPerColor:
    // このnewは配置newというもの
    // OSがメモリ管理できない状況なので、newは使えないが、メモリ領域上にインスタンスを作成して、コンストラクタを呼び出す。
    pixel_writer = new(pixel_writer_buf)
      RGBResv8BitPerColorPixelWriter{frame_buffer_config};
    break;
  case kPixelBGRResv8BitPerColor:
    pixel_writer = new(pixel_writer_buf)
      BGRResv8BitPerColorPixelWriter{frame_buffer_config};
    break;
  }

  const int kFrameWidth = frame_buffer_config.horizontal_resolution;
  const int kFrameHeight = frame_buffer_config.vertical_resolution;

  FillRectangle(*pixel_writer,
                {0, 0},
                {kFrameWidth, kFrameHeight - 50},
                kDesktopBGColor);
  FillRectangle(*pixel_writer,
                {0, kFrameHeight - 50},
                {kFrameWidth, 50},
                {243, 139, 160});
  FillRectangle(*pixel_writer,
                {0, kFrameHeight - 50},
                {kFrameWidth / 5, 50},
                {237, 246, 229});
  console = new(console_buf) Console{
    *pixel_writer, kDesktopFGColor, kDesktopBGColor
  };
  // 背景を丁寧に塗る
  /*for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x) {
    for (int y = 0; y < frame_buffer_config.vertical_resolution; ++y) {
      pixel_writer->Write(x, y, {143, 204, 232});
    }
  }
  // 円を丁寧に書く
  for (int x = 0; x < 400; ++x) {
    for (int y = 0; y < 400; ++y) {
      if ((x - 200)*(x - 200) + (y - 200)*(y - 200) <= 40000) {
        pixel_writer->Write(x, y, {234, 145, 152});
      }
    }
    }*/

  auto err = pci::ScanAllBus();
  printk("ScanAllBus: %s\n", err.Name());

  for (int i = 0; i < pci::num_device; ++i) {
    const auto& dev = pci::devices[i];
    auto vendor_id = pci::ReadVendorId(dev.bus, dev.device, dev.function);
    auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
    printk("%d.%d.%d: vend %04x, class %08x, head %02x\n",
        dev.bus, dev.device, dev.function,
        vendor_id, class_code, dev.header_type);
  }

  WriteString(*pixel_writer, 100, 100, "(:3", {0, 0, 0});
  for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
    for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
      if (mouse_cursor_shape[dy][dx] == '@') {
        pixel_writer->Write(200 + dx, 100 + dy, {255, 255, 255});
      } else if (mouse_cursor_shape[dy][dx] == '.') {
        pixel_writer->Write(200 + dx, 100 + dy, kDesktopFGColor);
      }
    }
  }
  while (1) __asm__("hlt");
}
