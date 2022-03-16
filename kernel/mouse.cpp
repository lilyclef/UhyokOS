#include "mouse.hpp"

#include "graphics.hpp"

namespace {
  const int tkMouseCursorWidth = 15;
  const int tkMouseCursorHeight = 24;
  const char tmouse_cursor_shape[tkMouseCursorHeight][tkMouseCursorWidth + 1] = {
    "@              ",
    "@@             ",
    "@.@            ",
    "@..@           ",
    "@...@          ",
    "@....@         ",
    "@.....@        ",
    "@......@       ",
    "@.......@      ",
    "@........@     ",
    "@.........@    ",
    "@..........@   ",
    "@...........@  ",
    "@............@ ",
    "@......@@@@@@@@",
    "@......@       ",
    "@....@@.@      ",
    "@...@ @.@      ",
    "@..@   @.@     ",
    "@.@    @.@     ",
    "@@      @.@    ",
    "@       @.@    ",
    "         @.@   ",
    "         @@@   ",
  };

  const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
    "    @.                       @      ",
    "  @.     @            @       @   @.",
    " @.     @.@          @.@       @  @.",
    "@.....   @            @        @  @.",
    "@@@@@@                         @  @.",
    "@.....        @.    @.         @  @.",
    "@@@@@@       @.  @. @.         @ @. ",
    " @.          @. @.@.@.        @ @.  ",
    "  @.         @@.  @@.        @@.    ",
    "    @.                      @       ",
  };
}

void DrawMouseCursor(PixelWriter* pixel_writer, Vector2D<int> position) {
  for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
    for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
      if (mouse_cursor_shape[dy][dx] == '@') {
        pixel_writer->Write(position.x + dx, position.y + dy, {142, 127, 127});
      } else if (mouse_cursor_shape[dy][dx] == '.') {
        pixel_writer->Write(position.x + dx, position.y + dy, {255, 255, 255});
      } else {
        pixel_writer->Write(position.x + dx, position.y + dy, kMouseTransparentColor);
      }

    }
  }
}
