#pragma once
#include "terminal/screen.hpp"
#include "terminal/style_pool.hpp"
#include "terminal/color.hpp"
#include "ui/layout.hpp"
#include <vector>
#include <string>
#include <optional>

namespace ccmake {

// Rendering operations collected from the component tree
struct RenderOp {
    enum Type { Write, Clear, Blit, Shift } type;

    // For Write: position and text content
    int x = 0, y = 0;
    std::string text;
    uint32_t style_id = 0;  // Interned style ID

    // For Clear: region
    struct { int x, y, w, h; } clear_rect{0, 0, 0, 0};

    // For Blit: source region copy
    // (skipped for now -- optimization for later)
};

// Output collector -- receives rendering operations and applies them to a Screen
class Output {
public:
    Output(int width, int height, StylePool& style_pool);

    // Set target screen
    void set_screen(Screen& screen);

    // Write text at position with style
    void write(int x, int y, const std::string& text, uint32_t style_id);

    // Clear a region
    void clear(int x, int y, int w, int h);

    // Get the resulting screen
    Screen& screen() { return screen_; }

private:
    int width_, height_;
    StylePool& style_pool_;
    Screen screen_;
};

}  // namespace ccmake
