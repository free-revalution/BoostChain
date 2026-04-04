#pragma once
#include <string>
#include <initializer_list>

namespace ccmake::ansi {

// Cursor movement
inline std::string cursor_up(int n = 1) { return "\x1b[" + std::to_string(n) + "A"; }
inline std::string cursor_down(int n = 1) { return "\x1b[" + std::to_string(n) + "B"; }
inline std::string cursor_forward(int n = 1) { return "\x1b[" + std::to_string(n) + "C"; }
inline std::string cursor_back(int n = 1) { return "\x1b[" + std::to_string(n) + "D"; }
inline std::string cursor_to(int row, int col) { return "\x1b[" + std::to_string(row) + ";" + std::to_string(col) + "H"; }
inline std::string cursor_to_column(int col) { return "\x1b[" + std::to_string(col) + "G"; }
inline std::string cursor_show() { return "\x1b[?25h"; }
inline std::string cursor_hide() { return "\x1b[?25l"; }
inline std::string cursor_save() { return "\x1b[s"; }
inline std::string cursor_restore() { return "\x1b[u"; }

// Screen manipulation
inline std::string clear_screen() { return "\x1b[2J"; }
inline std::string clear_line() { return "\x1b[2K"; }
inline std::string clear_line_to_end() { return "\x1b[0K"; }
inline std::string clear_line_to_start() { return "\x1b[1K"; }
inline std::string scroll_up(int n = 1) { return "\x1b[" + std::to_string(n) + "S"; }
inline std::string scroll_down(int n = 1) { return "\x1b[" + std::to_string(n) + "T"; }

// Text styling - SGR (Select Graphic Rendition)
inline std::string reset() { return "\x1b[0m"; }
inline std::string bold() { return "\x1b[1m"; }
inline std::string dim() { return "\x1b[2m"; }
inline std::string italic() { return "\x1b[3m"; }
inline std::string underline() { return "\x1b[4m"; }
inline std::string strikethrough() { return "\x1b[9m"; }
inline std::string inverse() { return "\x1b[7m"; }

// Colors (foreground)
inline std::string fg_rgb(int r, int g, int b) {
    return "\x1b[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}
inline std::string fg_256(int n) { return "\x1b[38;5;" + std::to_string(n) + "m"; }
inline std::string fg_default() { return "\x1b[39m"; }

// Colors (background)
inline std::string bg_rgb(int r, int g, int b) {
    return "\x1b[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}
inline std::string bg_256(int n) { return "\x1b[48;5;" + std::to_string(n) + "m"; }
inline std::string bg_default() { return "\x1b[49m"; }

// 16 standard ANSI foreground colors
inline std::string fg_black() { return "\x1b[30m"; }
inline std::string fg_red() { return "\x1b[31m"; }
inline std::string fg_green() { return "\x1b[32m"; }
inline std::string fg_yellow() { return "\x1b[33m"; }
inline std::string fg_blue() { return "\x1b[34m"; }
inline std::string fg_magenta() { return "\x1b[35m"; }
inline std::string fg_cyan() { return "\x1b[36m"; }
inline std::string fg_white() { return "\x1b[37m"; }
// Bright variants
inline std::string fg_bright_black() { return "\x1b[90m"; }
inline std::string fg_bright_red() { return "\x1b[91m"; }
inline std::string fg_bright_green() { return "\x1b[92m"; }
inline std::string fg_bright_yellow() { return "\x1b[93m"; }
inline std::string fg_bright_blue() { return "\x1b[94m"; }
inline std::string fg_bright_magenta() { return "\x1b[95m"; }
inline std::string fg_bright_cyan() { return "\x1b[96m"; }
inline std::string fg_bright_white() { return "\x1b[97m"; }

// 16 standard ANSI background colors
inline std::string bg_black() { return "\x1b[40m"; }
inline std::string bg_red() { return "\x1b[41m"; }
inline std::string bg_green() { return "\x1b[42m"; }
inline std::string bg_yellow() { return "\x1b[43m"; }
inline std::string bg_blue() { return "\x1b[44m"; }
inline std::string bg_magenta() { return "\x1b[45m"; }
inline std::string bg_cyan() { return "\x1b[46m"; }
inline std::string bg_white() { return "\x1b[47m"; }
// Bright
inline std::string bg_bright_black() { return "\x1b[100m"; }
inline std::string bg_bright_red() { return "\x1b[101m"; }
inline std::string bg_bright_green() { return "\x1b[102m"; }
inline std::string bg_bright_yellow() { return "\x1b[103m"; }
inline std::string bg_bright_blue() { return "\x1b[104m"; }
inline std::string bg_bright_magenta() { return "\x1b[105m"; }
inline std::string bg_bright_cyan() { return "\x1b[106m"; }
inline std::string bg_bright_white() { return "\x1b[107m"; }

// Terminal modes
inline std::string enable_alternate_screen() { return "\x1b[?1049h"; }
inline std::string disable_alternate_screen() { return "\x1b[?1049l"; }
inline std::string enable_mouse() { return "\x1b[?1000h"; }
inline std::string disable_mouse() { return "\x1b[?1000l"; }
inline std::string enable_bracketed_paste() { return "\x1b[?2004h"; }
inline std::string disable_bracketed_paste() { return "\x1b[?2004l"; }

// Scroll region
inline std::string set_scroll_region(int top, int bottom) {
    return "\x1b[" + std::to_string(top) + ";" + std::to_string(bottom) + "r";
}

// Erase
inline std::string erase_display() { return "\x1b[2J"; }

// Utility
inline std::string sgr(std::initializer_list<int> codes) {
    std::string s = "\x1b[";
    bool first = true;
    for (int c : codes) {
        if (!first) s += ";";
        s += std::to_string(c);
        first = false;
    }
    return s + "m";
}

}  // namespace ccmake::ansi
