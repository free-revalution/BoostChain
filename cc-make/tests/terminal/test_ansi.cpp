#include <catch2/catch_test_macros.hpp>
#include "terminal/ansi.hpp"

using namespace ccmake::ansi;

// ============================================================================
// Cursor movement
// ============================================================================

TEST_CASE("cursor_up produces correct escape sequence", "[ansi]") {
    REQUIRE(cursor_up() == "\x1b[1A");
    REQUIRE(cursor_up(5) == "\x1b[5A");
    REQUIRE(cursor_up(0) == "\x1b[0A");
    REQUIRE(cursor_up(100) == "\x1b[100A");
}

TEST_CASE("cursor_down produces correct escape sequence", "[ansi]") {
    REQUIRE(cursor_down() == "\x1b[1B");
    REQUIRE(cursor_down(3) == "\x1b[3B");
}

TEST_CASE("cursor_forward produces correct escape sequence", "[ansi]") {
    REQUIRE(cursor_forward() == "\x1b[1C");
    REQUIRE(cursor_forward(10) == "\x1b[10C");
}

TEST_CASE("cursor_back produces correct escape sequence", "[ansi]") {
    REQUIRE(cursor_back() == "\x1b[1D");
    REQUIRE(cursor_back(7) == "\x1b[7D");
}

TEST_CASE("cursor_to produces correct row;col sequence", "[ansi]") {
    REQUIRE(cursor_to(1, 1) == "\x1b[1;1H");
    REQUIRE(cursor_to(10, 20) == "\x1b[10;20H");
    REQUIRE(cursor_to(0, 0) == "\x1b[0;0H");
    REQUIRE(cursor_to(24, 80) == "\x1b[24;80H");
}

TEST_CASE("cursor_to_column produces correct sequence", "[ansi]") {
    REQUIRE(cursor_to_column(1) == "\x1b[1G");
    REQUIRE(cursor_to_column(40) == "\x1b[40G");
    REQUIRE(cursor_to_column(0) == "\x1b[0G");
}

TEST_CASE("cursor_show produces correct sequence", "[ansi]") {
    REQUIRE(cursor_show() == "\x1b[?25h");
}

TEST_CASE("cursor_hide produces correct sequence", "[ansi]") {
    REQUIRE(cursor_hide() == "\x1b[?25l");
}

TEST_CASE("cursor_save produces correct sequence", "[ansi]") {
    REQUIRE(cursor_save() == "\x1b[s");
}

TEST_CASE("cursor_restore produces correct sequence", "[ansi]") {
    REQUIRE(cursor_restore() == "\x1b[u");
}

// ============================================================================
// Screen manipulation
// ============================================================================

TEST_CASE("clear_screen produces correct sequence", "[ansi]") {
    REQUIRE(clear_screen() == "\x1b[2J");
}

TEST_CASE("erase_display is alias for clear_screen", "[ansi]") {
    REQUIRE(erase_display() == clear_screen());
}

TEST_CASE("clear_line produces correct sequence", "[ansi]") {
    REQUIRE(clear_line() == "\x1b[2K");
}

TEST_CASE("clear_line_to_end produces correct sequence", "[ansi]") {
    REQUIRE(clear_line_to_end() == "\x1b[0K");
}

TEST_CASE("clear_line_to_start produces correct sequence", "[ansi]") {
    REQUIRE(clear_line_to_start() == "\x1b[1K");
}

TEST_CASE("scroll_up produces correct sequence", "[ansi]") {
    REQUIRE(scroll_up() == "\x1b[1S");
    REQUIRE(scroll_up(5) == "\x1b[5S");
}

TEST_CASE("scroll_down produces correct sequence", "[ansi]") {
    REQUIRE(scroll_down() == "\x1b[1T");
    REQUIRE(scroll_down(3) == "\x1b[3T");
}

// ============================================================================
// Text styling - SGR
// ============================================================================

TEST_CASE("reset produces correct sequence", "[ansi]") {
    REQUIRE(reset() == "\x1b[0m");
}

TEST_CASE("bold produces correct sequence", "[ansi]") {
    REQUIRE(bold() == "\x1b[1m");
}

TEST_CASE("dim produces correct sequence", "[ansi]") {
    REQUIRE(dim() == "\x1b[2m");
}

TEST_CASE("italic produces correct sequence", "[ansi]") {
    REQUIRE(italic() == "\x1b[3m");
}

TEST_CASE("underline produces correct sequence", "[ansi]") {
    REQUIRE(underline() == "\x1b[4m");
}

TEST_CASE("strikethrough produces correct sequence", "[ansi]") {
    REQUIRE(strikethrough() == "\x1b[9m");
}

TEST_CASE("inverse produces correct sequence", "[ansi]") {
    REQUIRE(inverse() == "\x1b[7m");
}

// ============================================================================
// Foreground colors - RGB and 256
// ============================================================================

TEST_CASE("fg_rgb produces correct 38;2;r;g;b sequence", "[ansi]") {
    REQUIRE(fg_rgb(255, 0, 128) == "\x1b[38;2;255;0;128m");
    REQUIRE(fg_rgb(0, 0, 0) == "\x1b[38;2;0;0;0m");
    REQUIRE(fg_rgb(10, 20, 30) == "\x1b[38;2;10;20;30m");
}

TEST_CASE("fg_256 produces correct 38;5;n sequence", "[ansi]") {
    REQUIRE(fg_256(0) == "\x1b[38;5;0m");
    REQUIRE(fg_256(196) == "\x1b[38;5;196m");
    REQUIRE(fg_256(255) == "\x1b[38;5;255m");
}

TEST_CASE("fg_default produces correct sequence", "[ansi]") {
    REQUIRE(fg_default() == "\x1b[39m");
}

// ============================================================================
// Background colors - RGB and 256
// ============================================================================

TEST_CASE("bg_rgb produces correct 48;2;r;g;b sequence", "[ansi]") {
    REQUIRE(bg_rgb(255, 0, 128) == "\x1b[48;2;255;0;128m");
    REQUIRE(bg_rgb(0, 0, 0) == "\x1b[48;2;0;0;0m");
    REQUIRE(bg_rgb(10, 20, 30) == "\x1b[48;2;10;20;30m");
}

TEST_CASE("bg_256 produces correct 48;5;n sequence", "[ansi]") {
    REQUIRE(bg_256(0) == "\x1b[48;5;0m");
    REQUIRE(bg_256(196) == "\x1b[48;5;196m");
    REQUIRE(bg_256(255) == "\x1b[48;5;255m");
}

TEST_CASE("bg_default produces correct sequence", "[ansi]") {
    REQUIRE(bg_default() == "\x1b[49m");
}

// ============================================================================
// 16 standard ANSI foreground colors
// ============================================================================

TEST_CASE("standard ANSI foreground colors are correct", "[ansi]") {
    REQUIRE(fg_black() == "\x1b[30m");
    REQUIRE(fg_red() == "\x1b[31m");
    REQUIRE(fg_green() == "\x1b[32m");
    REQUIRE(fg_yellow() == "\x1b[33m");
    REQUIRE(fg_blue() == "\x1b[34m");
    REQUIRE(fg_magenta() == "\x1b[35m");
    REQUIRE(fg_cyan() == "\x1b[36m");
    REQUIRE(fg_white() == "\x1b[37m");
}

TEST_CASE("bright ANSI foreground colors are correct", "[ansi]") {
    REQUIRE(fg_bright_black() == "\x1b[90m");
    REQUIRE(fg_bright_red() == "\x1b[91m");
    REQUIRE(fg_bright_green() == "\x1b[92m");
    REQUIRE(fg_bright_yellow() == "\x1b[93m");
    REQUIRE(fg_bright_blue() == "\x1b[94m");
    REQUIRE(fg_bright_magenta() == "\x1b[95m");
    REQUIRE(fg_bright_cyan() == "\x1b[96m");
    REQUIRE(fg_bright_white() == "\x1b[97m");
}

// ============================================================================
// 16 standard ANSI background colors
// ============================================================================

TEST_CASE("standard ANSI background colors are correct", "[ansi]") {
    REQUIRE(bg_black() == "\x1b[40m");
    REQUIRE(bg_red() == "\x1b[41m");
    REQUIRE(bg_green() == "\x1b[42m");
    REQUIRE(bg_yellow() == "\x1b[43m");
    REQUIRE(bg_blue() == "\x1b[44m");
    REQUIRE(bg_magenta() == "\x1b[45m");
    REQUIRE(bg_cyan() == "\x1b[46m");
    REQUIRE(bg_white() == "\x1b[47m");
}

TEST_CASE("bright ANSI background colors are correct", "[ansi]") {
    REQUIRE(bg_bright_black() == "\x1b[100m");
    REQUIRE(bg_bright_red() == "\x1b[101m");
    REQUIRE(bg_bright_green() == "\x1b[102m");
    REQUIRE(bg_bright_yellow() == "\x1b[103m");
    REQUIRE(bg_bright_blue() == "\x1b[104m");
    REQUIRE(bg_bright_magenta() == "\x1b[105m");
    REQUIRE(bg_bright_cyan() == "\x1b[106m");
    REQUIRE(bg_bright_white() == "\x1b[107m");
}

// ============================================================================
// Terminal modes
// ============================================================================

TEST_CASE("alternate screen buffer enable/disable", "[ansi]") {
    REQUIRE(enable_alternate_screen() == "\x1b[?1049h");
    REQUIRE(disable_alternate_screen() == "\x1b[?1049l");
}

TEST_CASE("mouse tracking enable/disable", "[ansi]") {
    REQUIRE(enable_mouse() == "\x1b[?1000h");
    REQUIRE(disable_mouse() == "\x1b[?1000l");
}

TEST_CASE("bracketed paste enable/disable", "[ansi]") {
    REQUIRE(enable_bracketed_paste() == "\x1b[?2004h");
    REQUIRE(disable_bracketed_paste() == "\x1b[?2004l");
}

// ============================================================================
// Scroll region
// ============================================================================

TEST_CASE("set_scroll_region produces correct sequence", "[ansi]") {
    REQUIRE(set_scroll_region(1, 24) == "\x1b[1;24r");
    REQUIRE(set_scroll_region(5, 20) == "\x1b[5;20r");
    REQUIRE(set_scroll_region(0, 0) == "\x1b[0;0r");
}

// ============================================================================
// SGR utility
// ============================================================================

TEST_CASE("sgr produces correct multi-code sequence", "[ansi]") {
    REQUIRE(sgr({0}) == "\x1b[0m");
    REQUIRE(sgr({1, 31}) == "\x1b[1;31m");
    REQUIRE(sgr({1, 4, 32}) == "\x1b[1;4;32m");
    REQUIRE(sgr({38, 2, 255, 128, 0}) == "\x1b[38;2;255;128;0m");
    REQUIRE(sgr({}) == "\x1b[m");  // empty list still produces ESC[m
}

// ============================================================================
// All sequences start with ESC and have correct structure
// ============================================================================

TEST_CASE("all cursor movement sequences start with ESC[", "[ansi]") {
    REQUIRE(cursor_up().rfind("\x1b[", 0) == 0);
    REQUIRE(cursor_down().rfind("\x1b[", 0) == 0);
    REQUIRE(cursor_forward().rfind("\x1b[", 0) == 0);
    REQUIRE(cursor_back().rfind("\x1b[", 0) == 0);
    REQUIRE(cursor_to(1, 1).rfind("\x1b[", 0) == 0);
    REQUIRE(cursor_to_column(1).rfind("\x1b[", 0) == 0);
}

TEST_CASE("all SGR styling sequences end with 'm'", "[ansi]") {
    REQUIRE(reset().back() == 'm');
    REQUIRE(bold().back() == 'm');
    REQUIRE(dim().back() == 'm');
    REQUIRE(italic().back() == 'm');
    REQUIRE(underline().back() == 'm');
    REQUIRE(strikethrough().back() == 'm');
    REQUIRE(inverse().back() == 'm');
    REQUIRE(fg_rgb(1, 2, 3).back() == 'm');
    REQUIRE(fg_256(1).back() == 'm');
    REQUIRE(bg_rgb(1, 2, 3).back() == 'm');
    REQUIRE(bg_256(1).back() == 'm');
    REQUIRE(fg_black().back() == 'm');
    REQUIRE(bg_black().back() == 'm');
}
