#include "screen.hpp"

#include <algorithm>

namespace ccmake {

Screen::Screen(int width, int height)
    : width_(width), height_(height), cells_(static_cast<size_t>(width) * height) {}

void Screen::resize(int new_width, int new_height) {
    std::vector<Cell> new_cells(static_cast<size_t>(new_width) * new_height);

    // Copy overlapping region
    int copy_w = std::min(width_, new_width);
    int copy_h = std::min(height_, new_height);
    for (int y = 0; y < copy_h; ++y) {
        for (int x = 0; x < copy_w; ++x) {
            new_cells[y * new_width + x] = cells_[y * width_ + x];
        }
    }

    width_ = new_width;
    height_ = new_height;
    cells_ = std::move(new_cells);

    // Mark entire screen as damaged after resize
    damage_.expand(0, 0);
    damage_.expand(new_width - 1, new_height - 1);
}

const Cell& Screen::cell_at(int x, int y) const {
    static const Cell default_cell;
    if (!in_bounds(x, y)) return default_cell;
    return cells_[index(x, y)];
}

Cell& Screen::cell_at(int x, int y) {
    static Cell default_cell;
    if (!in_bounds(x, y)) return default_cell;
    return cells_[index(x, y)];
}

void Screen::set_cell(int x, int y, const Cell& cell) {
    if (!in_bounds(x, y)) return;
    cells_[index(x, y)] = cell;
    damage_.expand(x, y);
}

void Screen::clear_cell(int x, int y) {
    if (!in_bounds(x, y)) return;
    cells_[index(x, y)] = Cell{};
    damage_.expand(x, y);
}

void Screen::clear_region(int x, int y, int w, int h) {
    for (int row = y; row < y + h && row < height_; ++row) {
        for (int col = x; col < x + w && col < width_; ++col) {
            if (col >= 0 && row >= 0) {
                cells_[index(col, row)] = Cell{};
                damage_.expand(col, row);
            }
        }
    }
}

void Screen::clear() {
    std::fill(cells_.begin(), cells_.end(), Cell{});
    damage_.expand(0, 0);
    damage_.expand(width_ - 1, height_ - 1);
}

void Screen::fill(const Cell& cell) {
    std::fill(cells_.begin(), cells_.end(), cell);
    damage_.expand(0, 0);
    damage_.expand(width_ - 1, height_ - 1);
}

void diff_screens(const Screen& prev, const Screen& next, const DiffCallback& cb) {
    int w = std::min(prev.width(), next.width());
    int h = std::min(prev.height(), next.height());

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const Cell& p = prev.cell_at(x, y);
            const Cell& n = next.cell_at(x, y);

            if (p.codepoint != n.codepoint || p.style != n.style || p.width != n.width) {
                cb(x, y, p, n, p.style, n.style);
            }
        }
    }
}

}  // namespace ccmake
