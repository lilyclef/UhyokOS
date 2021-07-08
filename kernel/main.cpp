#include <cstdint>
#include <cstddef>
#include <cstdio>
#include "graphics.hpp"
#include "font.hpp"

// (:3 配置newを設定するための準備 begin
void* operator new(size_t size, void* buf) {
  return buf;
}

void operator delete(void* obj) noexcept {
}
// (:3 配置newを設定するための準備 end

// (:3 グローバル変数 begin
char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;
// (:3 グローバル変数 end


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

  // 背景を丁寧に塗る
  for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x) {
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
  }
  char buf[128];
  sprintf(buf, "1 + 2 = %d", 1 + 2);
  WriteString(*pixel_writer, 200, 82, buf, {0, 0, 0});

  WriteString(*pixel_writer, 100, 100, "(:3", {0, 0, 0});
  int i = 0;
  for (char c = '!'; c <= '~'; ++c, ++i) {
    WriteAscii(*pixel_writer, 8 * i, 50, c, {0, 0, 0});
  }
  while (1) __asm__("hlt");
}
