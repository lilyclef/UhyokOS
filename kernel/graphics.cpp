/**
 * @file graphics.cpp
 *
 * 画像描画関連のプログラムを集めたファイル．
 */

#include "graphics.hpp"

void RGBResv8BitPerColorPixelWriter::Write(Vector2D<int> pos, const PixelColor& c) {
  auto p = PixelAt(pos);
  p[0] = c.r;
  p[1] = c.g;
  p[2] = c.b;
}

void BGRResv8BitPerColorPixelWriter::Write(Vector2D<int> pos, const PixelColor& c) {
  auto p = PixelAt(pos);
  p[0] = c.b;
  p[1] = c.g;
  p[2] = c.r;
}

void DrawRectangle(PixelWriter& writer, const Vector2D<int>& pos,
                   const Vector2D<int>& size, const PixelColor& c) {
  for (int dx = 0; dx < size.x; ++dx) {
    writer.Write(pos + Vector2D<int>{dx, 0}, c);
    writer.Write(pos + Vector2D<int>{dx, size.y - 1}, c);
  }
  for (int dy = 1; dy < size.y - 1; ++dy) {
    writer.Write(pos + Vector2D<int>{0, dy}, c);
    writer.Write(pos + Vector2D<int>{size.x - 1, dy}, c);
  }
}

void FillRectangle(PixelWriter& writer, const Vector2D<int>& pos,
                   const Vector2D<int>& size, const PixelColor& c) {
  for (int dy = 0; dy < size.y; ++dy) {
    for (int dx = 0; dx < size.x; ++dx) {
      writer.Write(pos + Vector2D<int>{dx, dy}, c);
    }
  }
}

bool isRectangleRound(int x, int y, int max_x, int max_y, int r) {
  return !(r >= x + y) && !(y <= - max_x + x + r) && !(max_x + max_y - r <= x + y) && !(max_y - r + x <= y);
}

void FillRectangleRound(PixelWriter& writer, const Vector2D<int>& pos,
                   const Vector2D<int>& size, const PixelColor& c, int round) {
  for (int dy = 0; dy < size.y; ++dy) {
    for (int dx = 0; dx < size.x; ++dx) {
      if (isRectangleRound(dx, dy, size.x, size.y, round)) {
        writer.Write(pos + Vector2D<int>{dx, dy}, c);
      }
    }
  }
}

bool isRectangleRoundUpper(int x, int y, int max_x, int max_y, int r) {
  return !(r >= x + y) && !(y <= - max_x + x + r);
}

void FillRectangleRoundUpper(PixelWriter& writer, const Vector2D<int>& pos,
                   const Vector2D<int>& size, const PixelColor& c, int round) {
  for (int dy = 0; dy < size.y; ++dy) {
    for (int dx = 0; dx < size.x; ++dx) {
      if (isRectangleRoundUpper(dx, dy, size.x, size.y, round)) {
        writer.Write(pos + Vector2D<int>{dx, dy}, c);
      }
    }
  }
}


void DrawDesktop(PixelWriter& writer) {
  const auto width = writer.Width();
  const auto height = writer.Height();
  // Background
  FillRectangle(writer,
                {0, 0},
                {width, height},
                kDesktopBGColor);
  /*// Tool Bar
  FillRectangle(writer,
                {0, height - 50},
                {width, 50},
                {243, 139, 160});
  // Start Menu
  FillRectangle(writer,
                {0, height - 50},
                {width / 5, 50},
                {237, 246, 229});*/
}
