#include "ui/virtual_scroll.hpp"

namespace ccmake {

void ScrollViewport::scroll_by(int delta) {
    scroll_offset += delta;
    clamp();
}

void ScrollViewport::scroll_to_line(int line) {
    scroll_offset = total_lines - visible_lines - line;
    clamp();
}

void ScrollViewport::clamp() {
    int max_offset = total_lines - visible_lines;
    if (max_offset < 0) max_offset = 0;
    if (scroll_offset < 0) scroll_offset = 0;
    if (scroll_offset > max_offset) scroll_offset = max_offset;
}

bool ScrollViewport::is_visible(int line) const {
    int top = total_lines - visible_lines - scroll_offset;
    int bottom = top + visible_lines - 1;
    return line >= top && line <= bottom;
}

std::pair<int, int> ScrollViewport::visible_range() const {
    int top = total_lines - visible_lines - scroll_offset;
    int bottom = top + visible_lines - 1;
    return {top, bottom};
}

bool ScrollViewport::at_bottom() const {
    return scroll_offset == 0 || total_lines <= visible_lines;
}

int ScrollViewport::lines_above() const {
    int max_offset = total_lines - visible_lines;
    if (max_offset < 0) return 0;
    return scroll_offset;
}

int ScrollViewport::lines_below() const {
    int max_offset = total_lines - visible_lines;
    if (max_offset < 0) return 0;
    return max_offset - scroll_offset;
}

// ============================================================
// VirtualScrollManager
// ============================================================

void VirtualScrollManager::set_content_height(int total_lines) {
    viewport_.total_lines = total_lines;
    viewport_.clamp();
}

void VirtualScrollManager::set_viewport_height(int visible_lines) {
    viewport_.visible_lines = visible_lines;
    viewport_.clamp();
}

void VirtualScrollManager::page_up() {
    viewport_.scroll_by(viewport_.visible_lines / 2);
}

void VirtualScrollManager::page_down() {
    viewport_.scroll_by(-(viewport_.visible_lines / 2));
    viewport_.clamp();
}

void VirtualScrollManager::scroll_to_top() {
    int max_offset = viewport_.total_lines - viewport_.visible_lines;
    viewport_.scroll_offset = max_offset > 0 ? max_offset : 0;
}

void VirtualScrollManager::scroll_to_bottom() {
    viewport_.scroll_offset = 0;
}

const ScrollViewport& VirtualScrollManager::viewport() const {
    return viewport_;
}

}  // namespace ccmake
