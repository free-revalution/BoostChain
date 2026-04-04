#include <catch2/catch_test_macros.hpp>
#include "terminal/renderer.hpp"
#include "terminal/ansi.hpp"

using namespace ccmake;

// Helper: create a plain Frame of given dimensions
static Frame make_frame(int w, int h) {
    Frame f{.screen = Screen(w, h), .viewport_width = w, .viewport_height = h, .cursor = {}};
    return f;
}

// Helper: create a plain StylePool with a plain style interned as id=0
static StylePool make_pool() {
    StylePool pool;
    // Ensure the default "none" style is present
    TextStyle plain;
    pool.intern(plain);
    return pool;
}

// Helper: intern a bold style and return its id
static uint32_t bold_style_id(StylePool& pool) {
    TextStyle bold;
    bold.bold = true;
    return pool.intern(bold);
}

TEST_CASE("render_full_clear produces screen content and clear escape", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame f = make_frame(10, 5);
    // Set a few cells
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
    // Since no cells changed, the diff callback should never fire
    // We should only get cursor_show at most
    std::string without_cursor_vis = out.ansi;
    // Remove cursor show/hide escapes for comparison
    // "\x1b[?25h" is 6 chars: \x1b [ ? 2 5 h
    size_t pos;
    while ((pos = without_cursor_vis.find("\x1b[?25h")) != std::string::npos) {
        without_cursor_vis.erase(pos, 6);
    }
    while ((pos = without_cursor_vis.find("\x1b[?25l")) != std::string::npos) {
        without_cursor_vis.erase(pos, 6);
    }
    REQUIRE(without_cursor_vis.empty());
}

TEST_CASE("render detects single cell change", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(5, 5);
    Frame next = make_frame(5, 5);

    // Change one cell in next
    Cell c;
    c.codepoint = U'Q';
    next.screen.set_cell(3, 2, c);

    auto out = renderer.render(prev, next);

    REQUIRE_FALSE(out.ansi.empty());
    // Should contain the character 'Q'
    REQUIRE(out.ansi.find('Q') != std::string::npos);
    // Should NOT contain clear screen (not a full clear)
    REQUIRE(out.ansi.find("\x1b[2J") == std::string::npos);
    // Should contain cursor movement to position (3,2)
    // ANSI cursor position is 1-based: row 3, col 4
    REQUIRE(out.ansi.find("\x1b[3;4H") != std::string::npos);
}

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

TEST_CASE("render handles dimension change with full clear", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(10, 5);
    Frame next = make_frame(20, 10);

    // Set a cell in next
    Cell c;
    c.codepoint = U'R';
    next.screen.set_cell(15, 8, c);

    auto out = renderer.render(prev, next);

    REQUIRE_FALSE(out.ansi.empty());
    // Must contain clear screen since dimensions changed
    REQUIRE(out.ansi.find("\x1b[2J") != std::string::npos);
    // Must contain the character we set
    REQUIRE(out.ansi.find('R') != std::string::npos);
}

TEST_CASE("cursor positioning after render", "[renderer]") {
    StylePool pool = make_pool();
    LogUpdateRenderer renderer(pool);

    Frame prev = make_frame(10, 5);
    Frame next = make_frame(10, 5);

    // Set desired cursor position in the frame
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
    // Output cursor should match frame cursor
    REQUIRE(out.cursor.x == 5);
    REQUIRE(out.cursor.y == 3);
}
