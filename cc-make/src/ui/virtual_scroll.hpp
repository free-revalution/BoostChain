#pragma once

#include <string>
#include <vector>

namespace ccmake {

struct ScrollViewport {
    int total_lines = 0;
    int visible_lines = 24;
    int scroll_offset = 0;

    void scroll_by(int delta);
    void scroll_to_line(int line);
    void clamp();
    bool is_visible(int line) const;
    std::pair<int, int> visible_range() const;
    bool at_bottom() const;
    int lines_above() const;
    int lines_below() const;
};

class VirtualScrollManager {
public:
    void set_content_height(int total_lines);
    void set_viewport_height(int visible_lines);
    void page_up();
    void page_down();
    void scroll_to_top();
    void scroll_to_bottom();

    const ScrollViewport& viewport() const;

private:
    ScrollViewport viewport_;
};

}  // namespace ccmake
