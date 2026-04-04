#include "terminal/renderer.hpp"
#include "terminal/ansi.hpp"
#include <algorithm>

namespace ccmake {

// Convert a Unicode codepoint to its UTF-8 byte representation
static std::string codepoint_to_utf8(char32_t cp) {
    std::string result;
    if (cp <= 0x7F) {
        result += static_cast<char>(cp);
    } else if (cp <= 0x7FF) {
        result += static_cast<char>(0xC0 | (cp >> 6));
        result += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp <= 0xFFFF) {
        result += static_cast<char>(0xE0 | (cp >> 12));
        result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        result += static_cast<char>(0xF0 | (cp >> 18));
        result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (cp & 0x3F));
    }
    return result;
}

LogUpdateRenderer::LogUpdateRenderer(StylePool& style_pool)
    : style_pool_(style_pool) {}

std::string LogUpdateRenderer::render_cell(int x, int y, const Cell& prev_cell,
                                           const Cell& next_cell,
                                           uint32_t prev_style, uint32_t next_style,
                                           int& cursor_x, int& cursor_y,
                                           uint32_t& last_style_id) {
    (void)prev_cell;
    (void)prev_style;

    std::string out;

    // Skip SpacerTail cells -- they are the second column of a wide char
    // already rendered by the preceding Wide cell
    if (next_cell.width == CellWidth::SpacerTail) {
        return out;
    }

    // Move cursor to the target position if needed
    if (cursor_y != y || cursor_x != x) {
        // Use absolute positioning if we need to move both row and col,
        // or if the move is backwards
        if (cursor_y != y && cursor_x != x) {
            // ANSI cursor positioning is 1-based
            out += ansi::cursor_to(y + 1, x + 1);
        } else if (cursor_y != y) {
            int dy = y - cursor_y;
            if (dy > 0) {
                out += ansi::cursor_down(dy);
            } else {
                out += ansi::cursor_up(-dy);
            }
        } else {
            // Same row, different column
            int dx = x - cursor_x;
            if (dx > 0) {
                out += ansi::cursor_forward(dx);
            } else if (dx < 0) {
                out += ansi::cursor_back(-dx);
            }
        }
        cursor_x = x;
        cursor_y = y;
    }

    // Apply style transition if changed
    if (next_style != last_style_id) {
        out += style_pool_.transition(last_style_id, next_style);
        last_style_id = next_style;
    }

    // Write the codepoint as UTF-8
    out += codepoint_to_utf8(next_cell.codepoint);

    // Advance virtual cursor
    if (next_cell.width == CellWidth::Wide) {
        cursor_x += 2;
    } else {
        cursor_x += 1;
    }

    return out;
}

RenderOutput LogUpdateRenderer::render_full_clear(const Frame& frame) {
    RenderOutput result;
    result.cursor = frame.cursor;
    result.cursor_visible = frame.cursor.visible;

    int w = frame.screen.width();
    int h = frame.screen.height();

    // Clear screen
    result.ansi += ansi::clear_screen();

    // Write entire screen content row by row
    uint32_t last_style_id = style_pool_.none();

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const Cell& cell = frame.screen.cell_at(x, y);

            // Skip SpacerTail
            if (cell.width == CellWidth::SpacerTail) continue;

            // Apply style if different
            if (cell.style != last_style_id) {
                result.ansi += style_pool_.transition(last_style_id, cell.style);
                last_style_id = cell.style;
            }

            // Write codepoint
            result.ansi += codepoint_to_utf8(cell.codepoint);
        }
        // Newline at end of each row except the last
        if (y < h - 1) {
            result.ansi += "\r\n";
        }
    }

    // Position cursor at frame cursor location
    if (result.cursor.y > 0 || result.cursor.x > 0) {
        result.ansi += ansi::cursor_to(result.cursor.y + 1, result.cursor.x + 1);
    }

    // Cursor visibility
    if (result.cursor_visible) {
        result.ansi += ansi::cursor_show();
    } else {
        result.ansi += ansi::cursor_hide();
    }

    return result;
}

RenderOutput LogUpdateRenderer::render(const Frame& prev, const Frame& next, bool full_clear) {
    RenderOutput result;
    result.cursor = next.cursor;
    result.cursor_visible = next.cursor.visible;

    int prev_w = prev.screen.width();
    int prev_h = prev.screen.height();
    int next_w = next.screen.width();
    int next_h = next.screen.height();

    // Full clear path: dimensions changed or explicitly requested
    if (full_clear || prev_w != next_w || prev_h != next_h) {
        return render_full_clear(next);
    }

    // Standard diff path
    int cursor_x = 0;
    int cursor_y = 0;
    uint32_t last_style_id = style_pool_.none();

    diff_screens(prev.screen, next.screen, [&](int x, int y,
                const Cell& prev_cell, const Cell& next_cell,
                uint32_t prev_style, uint32_t next_style) {
        result.ansi += render_cell(x, y, prev_cell, next_cell,
                                   prev_style, next_style,
                                   cursor_x, cursor_y, last_style_id);
    });

    // Position cursor at the frame's desired cursor location
    if (result.cursor.y > 0 || result.cursor.x > 0) {
        result.ansi += ansi::cursor_to(result.cursor.y + 1, result.cursor.x + 1);
    }

    // Cursor visibility
    if (result.cursor_visible) {
        result.ansi += ansi::cursor_show();
    } else {
        result.ansi += ansi::cursor_hide();
    }

    return result;
}

}  // namespace ccmake
