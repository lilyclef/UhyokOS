#pragma once

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
  virtual void Write(int x, int y, const PixelColor& c) override;
};

// BGR表記
class BGRResv8BitPerColorPixelWriter : public PixelWriter {
public:
  using PixelWriter::PixelWriter;
  virtual void Write(int x, int y, const PixelColor& c) override;
};
