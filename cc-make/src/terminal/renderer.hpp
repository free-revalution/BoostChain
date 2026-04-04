#pragma once
#include "terminal/screen.hpp"
#include "terminal/style_pool.hpp"
#include <string>
#include <vector>

namespace ccmake {

// A frame represents the rendered output of a single UI update
struct Frame {
    Screen screen{0, 0};  // Default: empty screen
    int viewport_width = 0;
    int viewport_height = 0;
    Cursor cursor;
};

// Render diff result: the ANSI string to write to terminal
struct RenderOutput {
    std::string ansi;        // ANSI escape sequences to write
    Cursor cursor;           // Final cursor position
    bool cursor_visible = true;
};

// The main renderer - diffs two frames and produces terminal output
class LogUpdateRenderer {
public:
    explicit LogUpdateRenderer(StylePool& style_pool);

    // Diff two frames and produce ANSI output
    // prev: previous frame (may be empty for first render)
    // next: new frame to render
    // full_clear: if true, clear screen instead of diffing
    RenderOutput render(const Frame& prev, const Frame& next, bool full_clear = false);

    // Render a full screen clear
    RenderOutput render_full_clear(const Frame& frame);

private:
    StylePool& style_pool_;

    // Generate ANSI for writing a single cell at position
    std::string render_cell(int x, int y, const Cell& prev_cell, const Cell& next_cell,
                           uint32_t prev_style, uint32_t next_style,
                           int& cursor_x, int& cursor_y, uint32_t& last_style_id);
};

}  // namespace ccmake
