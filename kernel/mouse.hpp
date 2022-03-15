/**
 * @file mouse.hpp
 *
 * マウス制御プログラム．
 */

#pragma once

// [6.26] Definition of MouseCursor Class
#include "graphics.hpp"

const int kMouseCursorWidth = 36;
const int kMouseCursorHeight = 10;
const PixelColor kMouseTransparentColor{0, 0, 1};

void DrawMouseCursor(PixelWriter* pixel_writer, Vector2D<int> position);
