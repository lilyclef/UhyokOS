#include <cstdint>
#include <cstddef>
#include <cstdio>

#include <numeric>
#include <vector>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"
#include "console.hpp"
#include "pci.hpp"
#include "usb/memory.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"


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
  pci::Device* xhc_dev = nullptr;
  for (int i = 0; i < pci::num_device; ++i) {
    if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x30u)) {
      // xHC
      xhc_dev = &pci::devices[i];
      break;
    }
  }

  if (xhc_dev) {
    printk("xHC has been found: %d.%d.%d\n",
           xhc_dev->bus, xhc_dev->device, xhc_dev->function);
  }

  std::vector<int, usb::Allocator<int>> v;
  for (int i = 0; i <= 100; ++i) {
    v.push_back(i);
  }

  printk("sum: %d\n", std::accumulate(v.begin(), v.end(), 0));

  const auto bar = pci::ReadBar(*xhc_dev, 0);
  //const auto mmio_base = bitutil::ClearBits(bar.value, 0xf);
  const auto mmio_base = bar.value & ~static_cast<uint64_t>(0xf);
  printk("xHC mmio_base = %08lx\n", mmio_base);
  usb::xhci::Controller xhc{mmio_base};

  if (auto err = xhc.Initialize()) {
    printk("xhc.Initialize: %s\n", err.Name());
  }
  xhc.Run();

  printk("xHC start running\n");

  for (int i = 1; i <= xhc.MaxPorts(); ++i) {
    auto port = xhc.PortAt(i);
    printk("Port %d: IsConnected=%d\n", i, port.IsConnected());

    if (port.IsConnected()) {
      if (auto err = ConfigurePort(xhc, port)) {
        printk("failed to configure port: %s\n", err.Name());
        continue;
      }
    }
  }

  while (!xhc.PrimaryEventRing()->HasFront()) {
  }

  printk("xHC has least one event: %s\n", usb::xhci::kTRBTypeToName[
      xhc.PrimaryEventRing()->Front()->bits.trb_type]);

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

extern "C" void __cxa_pure_virtual() {
  while (1) __asm__("hlt");
}
