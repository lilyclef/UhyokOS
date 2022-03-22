#include "window.hpp"
#include "logger.hpp"
#include "font.hpp"

Window::Window(int width, int height, PixelFormat shadow_format) : width_{width}, height_{height} {
  data_.resize(height);
  for (int y = 0; y < height; ++y) {
    data_[y].resize(width);
  }
  FrameBufferConfig config{};
  config.frame_buffer = nullptr;
  config.horizontal_resolution = width;
  config.vertical_resolution = height;
  config.pixel_format = shadow_format;

  if (auto err = shadow_buffer_.Initialize(config)) {
    Log(kError, "failed to initialize shadow buffer: %s at %s:%d\n",
        err.Name(), err.File(), err.Line());
  }
}

void Window::DrawTo(FrameBuffer& dst, Vector2D<int> position) {
  if (!transparent_color_) {
    // Acceleration
    dst.Copy(position, shadow_buffer_);
    return;
  }

  const auto tc = transparent_color_.value();
  auto& writer = dst.Writer();
  for (int y = std::max(0, 0 - position.y); y < std::min(Height(), writer.Height() - position.y); ++y) {
    for (int x = std::max(0, 0 - position.x); x < std::min(Width(), writer.Width() - position.x); ++x) {
    const auto c = At(Vector2D<int>{x, y});
      if (c != tc) {
        writer.Write(position + Vector2D<int>{x, y}, c);
      }
    }
  }
}
// #@@range_end(window_drawto)

// #@@range_begin(window_settc)
void Window::SetTransparentColor(std::optional<PixelColor> c) {
  transparent_color_ = c;
}
// #@@range_end(window_settc)

Window::WindowWriter* Window::Writer() {
  return &writer_;
}

const PixelColor& Window::At(Vector2D<int> pos) const{
  return data_[pos.y][pos.x];
}

void Window::Write(Vector2D<int> pos, PixelColor c) {
  data_[pos.y][pos.x] = c;
  shadow_buffer_.Writer().Write(pos, c);
}

int Window::Width() const {
  return width_;
}

int Window::Height() const {
  return height_;
}

// [9.45] Shadow Buffer executes move 
void Window::Move(Vector2D<int> dst_pos, const Rectangle<int>& src) {
  shadow_buffer_.Move(dst_pos, src);
}

namespace {
  const int kCloseButtonWidth = 16;
  const int kCloseButtonHeight = 14;
  const char close_button[kCloseButtonHeight][kCloseButtonWidth + 1] = {
    "...............@",
    ".:::::::::::::$@",
    ".:::::::::::::$@",
    ".:::@@::::@@::$@",
    ".::::@@::@@:::$@",
    ".:::::@@@@::::$@",
    ".::::::@@:::::$@",
    ".:::::@@@@::::$@",
    ".::::@@::@@:::$@",
    ".:::@@::::@@::$@",
    ".:::::::::::::$@",
    ".:::::::::::::$@",
    ".$$$$$$$$$$$$$$@",
    "@@@@@@@@@@@@@@@@",
  };

  constexpr PixelColor ToColor(uint32_t c) {
    return {
      static_cast<uint8_t>((c >> 16) & 0xff),
      static_cast<uint8_t>((c >> 8) & 0xff),
      static_cast<uint8_t>(c & 0xff)
    };
  }
}
// #@@range_end(utils)

// #@@range_begin(draw_window)
void DrawWindow(PixelWriter& writer, const char* title) {
  auto fill_rect_round = [&writer](Vector2D<int> pos, Vector2D<int> size, uint32_t c) {
    FillRectangleRound(writer, pos, size, ToColor(c), 3);
  };
  auto fill_rect_round_upper = [&writer](Vector2D<int> pos, Vector2D<int> size, uint32_t c) {
    FillRectangleRoundUpper(writer, pos, size, ToColor(c), 3);
  };
  auto fill_rect = [&writer](Vector2D<int> pos, Vector2D<int> size, uint32_t c) {
    FillRectangle(writer, pos, size, ToColor(c));
  };
  const auto win_w = writer.Width();
  const auto win_h = writer.Height();

  // background
  fill_rect({0, 0}, {win_w, win_h}, 0xB5EAEA);
  // display
  fill_rect_round({0, 0}, {win_w, win_h}, 0xFFFBE9);
  // title bar
  fill_rect_round_upper({0, 0}, {win_w, 20}, 0xAD8B73);

  WriteString(writer, {24, 4}, title, ToColor(0xffffff));

  for (int y = 0; y < kCloseButtonHeight; ++y) {
    for (int x = 0; x < kCloseButtonWidth; ++x) {
      PixelColor c = ToColor(0xffffff);
      if (close_button[y][x] == '@') {
        c = ToColor(0xAD8B73);
      } else if (close_button[y][x] == '$') {
        c = ToColor(0x848484);
      } else if (close_button[y][x] == ':') {
        c = ToColor(0xFFFBE9);
      }
      writer.Write({win_w - 3 - kCloseButtonWidth + x, 3 + y}, c);
    }
  }
}
