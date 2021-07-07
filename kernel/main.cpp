#include <cstdint>
#include <cstddef>

#include "frame_buffer_config.hpp"

struct PixelColor {
  uint8_t r, g, b;
};

// (:3 PixelWriter begin
class PixelWriter {
public:
  PixelWriter(const FrameBufferConfig& config) : config_{config} {
  }
  virtual ~PixelWriter() = default;
  // プロトタイプ宣言の後ろに=0 : 純粋仮想関数 インターフェース的な
  virtual void Write(int x, int y, const PixelColor& c) = 0;
protected:
  uint8_t* PixelAt(int x, int y) {
    return config_.frame_buffer + 4 * (config_.pixels_per_scan_line * y + x);
  }
private:
  const FrameBufferConfig& config_;
};
// (:3 PixelWriterClass end


// インターフェースっぽく書かれたクラスを継承する子クラス=>使えるように。
// RGB表記
class RGBResv8BitPerColorPixelWriter : public PixelWriter {
public:
  // 親クラスのコンストラクタを子のコンストラクタとして扱える。
  using PixelWriter::PixelWriter;
  virtual void Write(int x, int y, const PixelColor& c) override {
    auto p = PixelAt(x, y);
    p[0] = c.r;
    p[1] = c.g;
    p[2] = c.b;
  }
};

// BGR表記
class BGRResv8BitPerColorPixelWriter : public PixelWriter {
public:
  using PixelWriter::PixelWriter;
  virtual void Write(int x, int y, const PixelColor& c) override {
    auto p = PixelAt(x, y);
    p[0] = c.b;
    p[1] = c.g;
    p[2] = c.r;
  }
};

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
  while (1) __asm__("hlt");
}
