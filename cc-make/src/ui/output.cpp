#include "ui/output.hpp"

namespace ccmake {

Output::Output(int width, int height, StylePool& style_pool)
    : width_(width), height_(height),
      style_pool_(style_pool),
      screen_(width, height) {}

void Output::set_screen(Screen& screen) {
    width_ = screen.width();
    height_ = screen.height();
    // We keep our own screen buffer
    screen_ = Screen(width_, height_);
}

void Output::write(int x, int y, const std::string& text, uint32_t style_id) {
    int cx = x;
    int cy = y;

    for (char ch : text) {
        if (ch == '\n') {
            cx = x;  // Reset to original x position
            ++cy;
            continue;
        }

        // Clamp to screen bounds
        if (cx >= 0 && cx < width_ && cy >= 0 && cy < height_) {
            Cell cell;
            cell.codepoint = static_cast<char32_t>(static_cast<unsigned char>(ch));
            cell.style = style_id;
            cell.width = CellWidth::Narrow;
            screen_.set_cell(cx, cy, cell);
        }

        ++cx;
    }
}

void Output::clear(int x, int y, int w, int h) {
    screen_.clear_region(x, y, w, h);
}

}  // namespace ccmake
