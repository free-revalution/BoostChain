#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace ccmake {

// Cell width (wide char support)
enum class CellWidth : uint8_t {
    Narrow = 0,      // Normal character
    Wide = 1,        // Double-width (CJK, emoji)
    SpacerTail = 2,  // Second column of wide char
    SpacerHead = 3   // Wide char at line end wrapping
};

// Interned style ID system
using StyleId = uint32_t;
constexpr StyleId STYLE_NONE = 0;

// A single cell in the screen buffer
struct Cell {
    char32_t codepoint = ' ';   // Unicode codepoint
    StyleId style = STYLE_NONE;  // Interned style ID
    CellWidth width = CellWidth::Narrow;
};

// Damage rectangle
struct Rect {
    int x = 0, y = 0;
    int width = 0, height = 0;

    bool is_empty() const { return width == 0 && height == 0; }

    void expand(int px, int py) {
        if (is_empty()) {
            x = px; y = py; width = 1; height = 1;
        } else {
            int x1 = std::min(x, px);
            int y1 = std::min(y, py);
            int x2 = std::max(x + width, px + 1);
            int y2 = std::max(y + height, py + 1);
            x = x1; y = y1; width = x2 - x1; height = y2 - y1;
        }
    }
};

// Frame cursor position
struct Cursor {
    int x = 0, y = 0;
    bool visible = true;
};

// The screen buffer - a 2D grid of cells
class Screen {
public:
    Screen(int width, int height);
    ~Screen() = default;

    // Screen dimensions
    int width() const { return width_; }
    int height() const { return height_; }

    // Resize screen
    void resize(int new_width, int new_height);

    // Cell access
    const Cell& cell_at(int x, int y) const;
    Cell& cell_at(int x, int y);

    // Cell manipulation
    void set_cell(int x, int y, const Cell& cell);
    void clear_cell(int x, int y);
    void clear_region(int x, int y, int w, int h);

    // Damage tracking
    const Rect& damage() const { return damage_; }
    void clear_damage() { damage_ = {}; }
    void mark_dirty(int x, int y) { damage_.expand(x, y); }

    // Clear entire screen
    void clear();

    // Fill screen with a cell
    void fill(const Cell& cell);

    // Check bounds
    bool in_bounds(int x, int y) const { return x >= 0 && x < width_ && y >= 0 && y < height_; }

private:
    int width_, height_;
    std::vector<Cell> cells_;
    Rect damage_;

    int index(int x, int y) const { return y * width_ + x; }
};

// Diff callback type
using DiffCallback = std::function<void(
    int x, int y,               // Cell position
    const Cell& prev_cell,      // Previous cell content
    const Cell& next_cell,      // New cell content
    StyleId prev_style,         // Previous style
    StyleId next_style          // New style
)>;

// Diff two screens and call callback for each changed cell
void diff_screens(const Screen& prev, const Screen& next, const DiffCallback& cb);

}  // namespace ccmake
