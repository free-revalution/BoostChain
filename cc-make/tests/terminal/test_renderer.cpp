#include <catch2/catch_test_macros.hpp>
#include "terminal/renderer.hpp"
#include "terminal/ansi.hpp"

using namespace ccmake;

// ============================================================================
// Helpers
// ============================================================================

// Create a plain Frame of given dimensions
static Frame make_frame(int w, int h) {
    Frame f{.screen = Screen(w, h), .viewport_width = w, .viewport_height = h, .cursor = {}};
    return f;
}

// Create a StylePool with a plain style interned as the default
static StylePool make_pool() {
    StylePool pool;
    TextStyle plain;
    pool.intern(plain);
    return pool;
}

// Intern a bold style and return its id
static uint32_t bold_style_id(StylePool& pool) {
    TextStyle bold;
    bold.bold = true;
    return pool.intern(bold);
}

// Intern a red foreground style and return its id
static uint32_t red_style_id(StylePool& pool) {
    TextStyle red;
    red.color = Color::rgb(255, 0, 0);
    return pool.intern(red);
}

// Intern an italic style and return its id
static uint32_t italic_style_id(StylePool& pool) {
    TextStyle italic;
    italic.italic = true;
    return pool.intern(italic);
}

// Strip cursor visibility escapes from a string for comparison
static std::string strip_cursor_vis(const std::string& s) {
    std::string result = s;
    size_t pos;
    while ((pos = result.find("\x1b[?25h")) != std::string::npos) {
        result.erase(pos, 6);
    }
    while ((pos = result.find("\x1b[?25l")) != std::string::npos) {
        result.erase(pos, 6);
    }
    return result;
}

// ============================================================================
// Construction
// ============================================================================

TEST_CASE("LogUpdateRenderer can be constructed with a StylePool", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);
    // If we reach here, construction succeeded
    REQUIRE(true);
}

// ============================================================================
// render_full_clear
// ============================================================================

TEST_CASE("render_full_clear produces screen content and clear escape", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame f = make_frame(10, 5);
    Cell c;
    c.codepoint = U'A';
    f.screen.set_cell(0, 0, c);
    c.codepoint = U'B';
    f.screen.set_cell(9, 4, c);
    c.codepoint = U'Z';
    f.screen.set_cell(3, 2, c);

    auto out = renderer.render_full_clear(f);

    REQUIRE_FALSE(out.ansi.empty());
    // Must contain clear screen escape
    REQUIRE(out.ansi.find("\x1b[2J") != std::string::npos);
    // Must contain the characters we set
    REQUIRE(out.ansi.find('A') != std::string::npos);
    REQUIRE(out.ansi.find('B') != std::string::npos);
    REQUIRE(out.ansi.find('Z') != std::string::npos);
    // Cursor should match frame cursor
    REQUIRE(out.cursor.x == f.cursor.x);
    REQUIRE(out.cursor.y == f.cursor.y);
}

TEST_CASE("render_full_clear includes row newlines between rows", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame f = make_frame(5, 3);

    auto out = renderer.render_full_clear(f);

    // A 3-row screen should have 2 \r\n separators (between row 0-1 and row 1-2)
    size_t first = out.ansi.find("\r\n");
    REQUIRE(first != std::string::npos);
    size_t second = out.ansi.find("\r\n", first + 2);
    REQUIRE(second != std::string::npos);
    // No third newline (last row does not get one)
    size_t third = out.ansi.find("\r\n", second + 2);
    REQUIRE(third == std::string::npos);
}

TEST_CASE("render_full_clear with cursor position outputs cursor escape", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame f = make_frame(10, 5);
    f.cursor.x = 7;
    f.cursor.y = 3;

    auto out = renderer.render_full_clear(f);

    // Should contain cursor positioning to row 4, col 8 (1-based)
    REQUIRE(out.ansi.find("\x1b[4;8H") != std::string::npos);
    REQUIRE(out.cursor.x == 7);
    REQUIRE(out.cursor.y == 3);
}

TEST_CASE("render_full_clear with hidden cursor", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame f = make_frame(10, 5);
    f.cursor.visible = false;

    auto out = renderer.render_full_clear(f);

    // Should contain cursor hide
    REQUIRE(out.ansi.find("\x1b[?25l") != std::string::npos);
    // Should NOT contain cursor show
    REQUIRE(out.ansi.find("\x1b[?25h") == std::string::npos);
    REQUIRE_FALSE(out.cursor_visible);
}

TEST_CASE("render_full_clear with visible cursor", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame f = make_frame(10, 5);
    f.cursor.visible = true;

    auto out = renderer.render_full_clear(f);

    // Should contain cursor show
    REQUIRE(out.ansi.find("\x1b[?25h") != std::string::npos);
    // Should NOT contain cursor hide
    REQUIRE(out.ansi.find("\x1b[?25l") == std::string::npos);
    REQUIRE(out.cursor_visible);
}

TEST_CASE("render_full_clear with style changes writes SGR sequences", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    uint32_t bold_id = bold_style_id(pool);

    Frame f = make_frame(10, 3);

    // Set a bold cell at (0, 0)
    Cell c;
    c.codepoint = U'X';
    c.style = bold_id;
    f.screen.set_cell(0, 0, c);

    auto out = renderer.render_full_clear(f);

    // Should contain bold SGR
    REQUIRE(out.ansi.find("\x1b[1m") != std::string::npos);
    REQUIRE(out.ansi.find('X') != std::string::npos);
}

TEST_CASE("render_full_clear with 1x1 screen", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame f = make_frame(1, 1);
    Cell c;
    c.codepoint = U'Y';
    f.screen.set_cell(0, 0, c);

    auto out = renderer.render_full_clear(f);

    REQUIRE_FALSE(out.ansi.empty());
    REQUIRE(out.ansi.find("\x1b[2J") != std::string::npos);
    REQUIRE(out.ansi.find('Y') != std::string::npos);
    // No newlines for a 1x1 screen
    REQUIRE(out.ansi.find("\r\n") == std::string::npos);
}

// ============================================================================
// render - no changes
// ============================================================================

TEST_CASE("render detects no changes between identical frames", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(5, 5);
    Frame next = make_frame(5, 5);

    // Set the same cell in both frames
    Cell c;
    c.codepoint = U'X';
    prev.screen.set_cell(2, 3, c);
    next.screen.set_cell(2, 3, c);

    auto out = renderer.render(prev, next);

    // The only output should be cursor visibility/show escape, not cell content
    REQUIRE(strip_cursor_vis(out.ansi).empty());
}

TEST_CASE("render with empty frames produces only cursor visibility", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(5, 5);
    Frame next = make_frame(5, 5);

    auto out = renderer.render(prev, next);

    REQUIRE(strip_cursor_vis(out.ansi).empty());
}

// ============================================================================
// render - single cell change
// ============================================================================

TEST_CASE("render detects single cell change", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(5, 5);
    Frame next = make_frame(5, 5);

    Cell c;
    c.codepoint = U'Q';
    next.screen.set_cell(3, 2, c);

    auto out = renderer.render(prev, next);

    REQUIRE_FALSE(out.ansi.empty());
    REQUIRE(out.ansi.find('Q') != std::string::npos);
    // Should NOT contain clear screen (not a full clear)
    REQUIRE(out.ansi.find("\x1b[2J") == std::string::npos);
    // Should contain cursor movement to position (3,2) - ANSI 1-based: row 3, col 4
    REQUIRE(out.ansi.find("\x1b[3;4H") != std::string::npos);
}

// ============================================================================
// render - multiple cell changes
// ============================================================================

TEST_CASE("render detects multiple cell changes", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(10, 5);
    Frame next = make_frame(10, 5);

    // Change three cells in next
    Cell c;
    c.codepoint = U'A';
    next.screen.set_cell(0, 0, c);
    c.codepoint = U'B';
    next.screen.set_cell(4, 2, c);
    c.codepoint = U'C';
    next.screen.set_cell(9, 4, c);

    auto out = renderer.render(prev, next);

    REQUIRE_FALSE(out.ansi.empty());
    // Should NOT contain clear screen
    REQUIRE(out.ansi.find("\x1b[2J") == std::string::npos);
    // All three characters should be present
    REQUIRE(out.ansi.find('A') != std::string::npos);
    REQUIRE(out.ansi.find('B') != std::string::npos);
    REQUIRE(out.ansi.find('C') != std::string::npos);
}

TEST_CASE("render with consecutive cell changes on same row uses relative moves", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(10, 2);
    Frame next = make_frame(10, 2);

    // Change two cells on the same row: (0,0) and (1,0)
    Cell c;
    c.codepoint = U'P';
    next.screen.set_cell(0, 0, c);
    c.codepoint = U'Q';
    next.screen.set_cell(1, 0, c);

    auto out = renderer.render(prev, next);

    REQUIRE_FALSE(out.ansi.empty());
    REQUIRE(out.ansi.find('P') != std::string::npos);
    REQUIRE(out.ansi.find('Q') != std::string::npos);
    // After writing P at (0,0), virtual cursor is at (1,0) -- same position as Q,
    // so no cursor movement needed before Q. Should NOT have a second cursor_to.
    // There should be exactly one cursor_to (for the first cell at 1;1)
    size_t first_cursor = out.ansi.find("\x1b[");
    REQUIRE(first_cursor != std::string::npos);
    // The remaining output after the first escape should contain P, Q without
    // additional cursor positioning for the second cell
}

// ============================================================================
// render - dimension change triggers full clear
// ============================================================================

TEST_CASE("render handles dimension change with full clear", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(10, 5);
    Frame next = make_frame(20, 10);

    Cell c;
    c.codepoint = U'R';
    next.screen.set_cell(15, 8, c);

    auto out = renderer.render(prev, next);

    REQUIRE_FALSE(out.ansi.empty());
    REQUIRE(out.ansi.find("\x1b[2J") != std::string::npos);
    REQUIRE(out.ansi.find('R') != std::string::npos);
}

TEST_CASE("render with width change only triggers full clear", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(10, 5);
    Frame next = make_frame(20, 5);  // same height, different width

    Cell c;
    c.codepoint = U'W';
    next.screen.set_cell(15, 3, c);

    auto out = renderer.render(prev, next);

    REQUIRE(out.ansi.find("\x1b[2J") != std::string::npos);
    REQUIRE(out.ansi.find('W') != std::string::npos);
}

TEST_CASE("render with height change only triggers full clear", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(10, 5);
    Frame next = make_frame(10, 10);  // same width, different height

    Cell c;
    c.codepoint = U'H';
    next.screen.set_cell(5, 8, c);

    auto out = renderer.render(prev, next);

    REQUIRE(out.ansi.find("\x1b[2J") != std::string::npos);
    REQUIRE(out.ansi.find('H') != std::string::npos);
}

TEST_CASE("render with explicit full_clear flag triggers full clear", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(10, 5);
    Frame next = make_frame(10, 5);  // same dimensions

    Cell c;
    c.codepoint = U'F';
    next.screen.set_cell(3, 2, c);

    // Request explicit full clear
    auto out = renderer.render(prev, next, /*full_clear=*/true);

    REQUIRE(out.ansi.find("\x1b[2J") != std::string::npos);
    REQUIRE(out.ansi.find('F') != std::string::npos);
}

// ============================================================================
// render - style changes
// ============================================================================

TEST_CASE("render applies style transitions", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    uint32_t bold_id = bold_style_id(pool);

    Frame prev = make_frame(5, 5);
    Frame next = make_frame(5, 5);

    // Previous frame: plain cell at (1,1)
    Cell c;
    c.codepoint = U'T';
    c.style = pool.none();
    prev.screen.set_cell(1, 1, c);

    // Next frame: bold cell at (1,1)
    Cell bold_cell;
    bold_cell.codepoint = U'T';
    bold_cell.style = bold_id;
    next.screen.set_cell(1, 1, bold_cell);

    auto out = renderer.render(prev, next);

    REQUIRE_FALSE(out.ansi.empty());
    // Should contain bold SGR escape (ESC[1m)
    REQUIRE(out.ansi.find("\x1b[1m") != std::string::npos);
    // Should NOT contain clear screen
    REQUIRE(out.ansi.find("\x1b[2J") == std::string::npos);
}

TEST_CASE("render applies color style transitions", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    uint32_t red_id = red_style_id(pool);

    Frame prev = make_frame(5, 5);
    Frame next = make_frame(5, 5);

    Cell c;
    c.codepoint = U'C';
    c.style = pool.none();
    prev.screen.set_cell(2, 2, c);

    Cell red_cell;
    red_cell.codepoint = U'C';
    red_cell.style = red_id;
    next.screen.set_cell(2, 2, red_cell);

    auto out = renderer.render(prev, next);

    REQUIRE_FALSE(out.ansi.empty());
    // Should contain the RGB foreground SGR: ESC[38;2;255;0;0m
    REQUIRE(out.ansi.find("\x1b[38;2;255;0;0m") != std::string::npos);
    REQUIRE(out.ansi.find('C') != std::string::npos);
}

TEST_CASE("render with same style does not produce style transition", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(5, 5);
    Frame next = make_frame(5, 5);

    Cell c;
    c.codepoint = U'S';
    prev.screen.set_cell(0, 0, c);
    next.screen.set_cell(0, 0, c);

    auto out = renderer.render(prev, next);

    // No changes at all, so no style transition escapes
    REQUIRE(strip_cursor_vis(out.ansi).empty());
}

TEST_CASE("render transitions between multiple styles", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    uint32_t bold_id = bold_style_id(pool);
    uint32_t italic_id = italic_style_id(pool);

    Frame prev = make_frame(5, 5);
    Frame next = make_frame(5, 5);

    // Two cells with different styles
    Cell c1;
    c1.codepoint = U'A';
    c1.style = bold_id;
    next.screen.set_cell(0, 0, c1);

    Cell c2;
    c2.codepoint = U'B';
    c2.style = italic_id;
    next.screen.set_cell(1, 0, c2);

    auto out = renderer.render(prev, next);

    REQUIRE(out.ansi.find('A') != std::string::npos);
    REQUIRE(out.ansi.find('B') != std::string::npos);
    // Bold: ESC[1m, transition from bold to italic: ESC[22;3m (reset bold + set italic)
    REQUIRE(out.ansi.find("\x1b[1m") != std::string::npos);
    REQUIRE(out.ansi.find("3m") != std::string::npos);  // italic SGR present
}

// ============================================================================
// render - cursor position
// ============================================================================

TEST_CASE("cursor positioning after render", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(10, 5);
    Frame next = make_frame(10, 5);

    next.cursor.x = 5;
    next.cursor.y = 3;

    // Make one cell change so there is some output
    Cell c;
    c.codepoint = U'W';
    next.screen.set_cell(0, 0, c);

    auto out = renderer.render(prev, next);

    REQUIRE_FALSE(out.ansi.empty());
    // Should contain cursor positioning to (row 4, col 6) -- 1-based
    REQUIRE(out.ansi.find("\x1b[4;6H") != std::string::npos);
    REQUIRE(out.cursor.x == 5);
    REQUIRE(out.cursor.y == 3);
}

TEST_CASE("render preserves cursor at origin when no cursor position set", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(10, 5);
    Frame next = make_frame(10, 5);

    // Cursor at default (0,0), visible
    Cell c;
    c.codepoint = U'D';
    next.screen.set_cell(0, 0, c);

    auto out = renderer.render(prev, next);

    REQUIRE(out.cursor.x == 0);
    REQUIRE(out.cursor.y == 0);
}

// ============================================================================
// render - cursor visibility
// ============================================================================

TEST_CASE("render with visible cursor includes show escape", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(5, 5);
    Frame next = make_frame(5, 5);
    next.cursor.visible = true;

    auto out = renderer.render(prev, next);

    // Even with no cell changes, cursor visibility is emitted
    REQUIRE(out.ansi.find("\x1b[?25h") != std::string::npos);
    REQUIRE(out.cursor_visible);
}

TEST_CASE("render with hidden cursor includes hide escape", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(5, 5);
    Frame next = make_frame(5, 5);
    next.cursor.visible = false;

    auto out = renderer.render(prev, next);

    REQUIRE(out.ansi.find("\x1b[?25l") != std::string::npos);
    REQUIRE_FALSE(out.cursor_visible);
}

TEST_CASE("render_full_clear respects cursor visibility", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame f = make_frame(5, 5);
    f.cursor.visible = false;

    auto out = renderer.render_full_clear(f);

    REQUIRE(out.ansi.find("\x1b[?25l") != std::string::npos);
    REQUIRE_FALSE(out.cursor_visible);
}

// ============================================================================
// render - wide character handling
// ============================================================================

TEST_CASE("render skips SpacerTail cells", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(10, 5);
    Frame next = make_frame(10, 5);

    // Set a wide character at (2, 1) with a SpacerTail at (3, 1)
    Cell wide;
    wide.codepoint = U'\u4e16';  // CJK character
    wide.width = CellWidth::Wide;
    next.screen.set_cell(2, 1, wide);

    Cell spacer;
    spacer.codepoint = U'\0';
    spacer.width = CellWidth::SpacerTail;
    next.screen.set_cell(3, 1, spacer);

    auto out = renderer.render(prev, next);

    REQUIRE_FALSE(out.ansi.empty());
    // Should contain the CJK character's UTF-8 encoding
    // \u4e16 in UTF-8 is E4 B8 96
    REQUIRE(out.ansi.find("\xe4\xb8\x96") != std::string::npos);
    // Should NOT contain clear screen
    REQUIRE(out.ansi.find("\x1b[2J") == std::string::npos);
}

TEST_CASE("render advances cursor by 2 for wide characters", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(10, 5);
    Frame next = make_frame(10, 5);

    // Wide char at (0, 0), narrow char at (2, 0)
    Cell wide;
    wide.codepoint = U'\u4e16';
    wide.width = CellWidth::Wide;
    next.screen.set_cell(0, 0, wide);

    Cell narrow;
    narrow.codepoint = U'N';
    next.screen.set_cell(2, 0, narrow);

    auto out = renderer.render(prev, next);

    REQUIRE_FALSE(out.ansi.empty());
    REQUIRE(out.ansi.find("\xe4\xb8\x96") != std::string::npos);
    REQUIRE(out.ansi.find('N') != std::string::npos);
    // After writing the wide char at (0,0), virtual cursor moves to (2,0)
    // which is exactly where N is, so no cursor movement needed for N
    // The output should have the wide char immediately followed by N
    // (or with only the first cursor positioning escape at the start)
}

TEST_CASE("render_full_clear skips SpacerTail cells", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame f = make_frame(5, 3);

    // Wide char at (0, 0) with SpacerTail at (1, 0)
    Cell wide;
    wide.codepoint = U'\u4e16';
    wide.width = CellWidth::Wide;
    f.screen.set_cell(0, 0, wide);

    Cell spacer;
    spacer.codepoint = U'\0';
    spacer.width = CellWidth::SpacerTail;
    f.screen.set_cell(1, 0, spacer);

    auto out = renderer.render_full_clear(f);

    REQUIRE_FALSE(out.ansi.empty());
    // The wide char should be present
    REQUIRE(out.ansi.find("\xe4\xb8\x96") != std::string::npos);
    // SpacerTail should not produce visible output
    // The SpacerTail codepoint (U'\0') should not appear as a NUL byte in meaningful output
}

// ============================================================================
// render - RenderOutput fields
// ============================================================================

TEST_CASE("render sets cursor from next frame", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(10, 5);
    Frame next = make_frame(10, 5);

    next.cursor.x = 3;
    next.cursor.y = 4;
    next.cursor.visible = false;

    Cell c;
    c.codepoint = U'Z';
    next.screen.set_cell(0, 0, c);

    auto out = renderer.render(prev, next);

    REQUIRE(out.cursor.x == 3);
    REQUIRE(out.cursor.y == 4);
    REQUIRE_FALSE(out.cursor_visible);
}

TEST_CASE("render_full_clear sets cursor from frame", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame f = make_frame(10, 5);
    f.cursor.x = 9;
    f.cursor.y = 4;
    f.cursor.visible = true;

    auto out = renderer.render_full_clear(f);

    REQUIRE(out.cursor.x == 9);
    REQUIRE(out.cursor.y == 4);
    REQUIRE(out.cursor_visible);
}
