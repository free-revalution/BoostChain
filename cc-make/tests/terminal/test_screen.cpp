#include <catch2/catch_test_macros.hpp>
#include "terminal/screen.hpp"

using namespace ccmake;

// ============================================================================
// Screen construction
// ============================================================================

TEST_CASE("Screen creation has correct dimensions", "[screen]") {
    Screen s(80, 24);
    REQUIRE(s.width() == 80);
    REQUIRE(s.height() == 24);
    REQUIRE(s.in_bounds(0, 0));
    REQUIRE(s.in_bounds(79, 23));
    REQUIRE_FALSE(s.in_bounds(80, 24));
    REQUIRE_FALSE(s.in_bounds(-1, 0));
}

TEST_CASE("Screen default cells are space with STYLE_NONE", "[screen]") {
    Screen s(10, 10);
    const Cell& c = s.cell_at(0, 0);
    REQUIRE(c.codepoint == U' ');
    REQUIRE(c.style == STYLE_NONE);
    REQUIRE(c.width == CellWidth::Narrow);
}

TEST_CASE("Screen 1x1 is valid", "[screen]") {
    Screen s(1, 1);
    REQUIRE(s.width() == 1);
    REQUIRE(s.height() == 1);
    REQUIRE(s.in_bounds(0, 0));
    REQUIRE_FALSE(s.in_bounds(1, 0));
    REQUIRE_FALSE(s.in_bounds(0, 1));
}

// ============================================================================
// Cell access
// ============================================================================

TEST_CASE("set_cell and cell_at work correctly", "[screen]") {
    Screen s(10, 10);

    Cell c;
    c.codepoint = U'A';
    c.style = 42;
    c.width = CellWidth::Narrow;

    s.set_cell(3, 5, c);

    const Cell& read = s.cell_at(3, 5);
    REQUIRE(read.codepoint == U'A');
    REQUIRE(read.style == 42);
    REQUIRE(read.width == CellWidth::Narrow);

    // Unmodified cell should be default (space)
    const Cell& def = s.cell_at(0, 0);
    REQUIRE(def.codepoint == U' ');
    REQUIRE(def.style == STYLE_NONE);
}

TEST_CASE("set_cell with wide character", "[screen]") {
    Screen s(10, 10);
    Cell c;
    c.codepoint = U'\u4e16';  // CJK character
    c.width = CellWidth::Wide;

    s.set_cell(5, 2, c);
    REQUIRE(s.cell_at(5, 2).codepoint == U'\u4e16');
    REQUIRE(s.cell_at(5, 2).width == CellWidth::Wide);
}

TEST_CASE("mutable cell_at allows direct modification", "[screen]") {
    Screen s(10, 10);
    Cell& c = s.cell_at(4, 3);
    c.codepoint = U'X';
    c.style = 77;
    REQUIRE(s.cell_at(4, 3).codepoint == U'X');
    REQUIRE(s.cell_at(4, 3).style == 77);
}

// ============================================================================
// Cell manipulation
// ============================================================================

TEST_CASE("clear_cell resets to default", "[screen]") {
    Screen s(10, 10);

    Cell c;
    c.codepoint = U'X';
    c.style = 99;
    s.set_cell(2, 3, c);

    REQUIRE(s.cell_at(2, 3).codepoint == U'X');

    s.clear_cell(2, 3);

    REQUIRE(s.cell_at(2, 3).codepoint == U' ');
    REQUIRE(s.cell_at(2, 3).style == STYLE_NONE);
    REQUIRE(s.cell_at(2, 3).width == CellWidth::Narrow);
}

TEST_CASE("clear_region clears specified area", "[screen]") {
    Screen s(10, 10);

    Cell c;
    c.codepoint = U'Z';
    c.style = 1;

    // Fill a 3x3 region
    for (int y = 2; y < 5; ++y) {
        for (int x = 2; x < 5; ++x) {
            s.set_cell(x, y, c);
        }
    }

    // Clear center 3x3 region (overlapping with the filled area)
    s.clear_region(3, 3, 3, 3);

    REQUIRE(s.cell_at(2, 2).codepoint == U'Z'); // outside clear region, untouched
    REQUIRE(s.cell_at(3, 3).codepoint == U' '); // inside clear region, cleared
    REQUIRE(s.cell_at(5, 5).codepoint == U' '); // inside clear region, cleared
}

TEST_CASE("clear clears entire screen", "[screen]") {
    Screen s(10, 10);

    Cell c;
    c.codepoint = U'Q';
    s.fill(c);

    REQUIRE(s.cell_at(0, 0).codepoint == U'Q');
    REQUIRE(s.cell_at(9, 9).codepoint == U'Q');

    s.clear();

    REQUIRE(s.cell_at(0, 0).codepoint == U' ');
    REQUIRE(s.cell_at(9, 9).codepoint == U' ');
}

TEST_CASE("fill sets all cells to given value", "[screen]") {
    Screen s(5, 3);

    Cell c;
    c.codepoint = U'F';
    c.style = 10;
    c.width = CellWidth::Narrow;
    s.fill(c);

    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 5; ++x) {
            REQUIRE(s.cell_at(x, y).codepoint == U'F');
            REQUIRE(s.cell_at(x, y).style == 10);
        }
    }
}

// ============================================================================
// Bounds checking
// ============================================================================

TEST_CASE("out of bounds access returns safe default", "[screen]") {
    Screen s(10, 10);

    // Accessing out of bounds should not crash, and return a default cell
    const Cell& c1 = s.cell_at(-1, 0);
    REQUIRE(c1.codepoint == U' ');
    REQUIRE(c1.style == STYLE_NONE);

    const Cell& c2 = s.cell_at(10, 10);
    REQUIRE(c2.codepoint == U' ');
    REQUIRE(c2.style == STYLE_NONE);

    // Setting out of bounds should be a no-op
    Cell c;
    c.codepoint = U'!';
    s.set_cell(-1, -1, c);
    REQUIRE(s.cell_at(-1, -1).codepoint == U' '); // unchanged

    REQUIRE_FALSE(s.in_bounds(-1, 0));
    REQUIRE_FALSE(s.in_bounds(10, 0));
    REQUIRE_FALSE(s.in_bounds(0, 10));
    REQUIRE_FALSE(s.in_bounds(-1, -1));
}

// ============================================================================
// Resize
// ============================================================================

TEST_CASE("resize preserves existing content in overlap region", "[screen]") {
    Screen s(5, 5);

    Cell c;
    c.codepoint = U'R';
    c.style = 7;
    s.set_cell(2, 2, c);
    s.set_cell(4, 4, c);

    // Grow the screen
    s.resize(10, 10);

    REQUIRE(s.width() == 10);
    REQUIRE(s.height() == 10);
    REQUIRE(s.cell_at(2, 2).codepoint == U'R');
    REQUIRE(s.cell_at(4, 4).codepoint == U'R');
    // New areas should be default
    REQUIRE(s.cell_at(5, 5).codepoint == U' ');
}

TEST_CASE("resize shrinks screen and truncates content", "[screen]") {
    Screen s(10, 10);

    Cell c;
    c.codepoint = U'S';
    s.set_cell(8, 8, c);
    s.set_cell(2, 2, c);

    // Shrink the screen
    s.resize(5, 5);

    REQUIRE(s.width() == 5);
    REQUIRE(s.height() == 5);
    // Content in overlap should be preserved
    REQUIRE(s.cell_at(2, 2).codepoint == U'S');
    // Content outside new bounds is lost (but should not crash)
}

TEST_CASE("resize marks entire screen as damaged", "[screen]") {
    Screen s(10, 10);
    s.clear_damage();
    REQUIRE(s.damage().is_empty());

    s.resize(20, 20);
    REQUIRE_FALSE(s.damage().is_empty());
    // Damage should cover at least the new dimensions
    REQUIRE(s.damage().width > 0);
    REQUIRE(s.damage().height > 0);
}

// ============================================================================
// Damage tracking
// ============================================================================

TEST_CASE("damage tracking marks dirty cells", "[screen]") {
    Screen s(20, 20);

    REQUIRE(s.damage().is_empty());

    s.mark_dirty(5, 5);
    REQUIRE_FALSE(s.damage().is_empty());
    REQUIRE(s.damage().x == 5);
    REQUIRE(s.damage().y == 5);
    REQUIRE(s.damage().width == 1);
    REQUIRE(s.damage().height == 1);

    s.mark_dirty(10, 15);
    REQUIRE(s.damage().x == 5);
    REQUIRE(s.damage().y == 5);
    REQUIRE(s.damage().width == 6);   // 10 - 5 + 1
    REQUIRE(s.damage().height == 11); // 15 - 5 + 1

    s.clear_damage();
    REQUIRE(s.damage().is_empty());

    // set_cell should also mark dirty
    Cell c;
    c.codepoint = U'D';
    s.set_cell(3, 3, c);
    REQUIRE_FALSE(s.damage().is_empty());
}

TEST_CASE("clear and fill mark damage", "[screen]") {
    Screen s(10, 10);
    s.clear_damage();

    s.clear();
    REQUIRE_FALSE(s.damage().is_empty());

    s.clear_damage();

    Cell c;
    c.codepoint = U'X';
    s.fill(c);
    REQUIRE_FALSE(s.damage().is_empty());
}

// ============================================================================
// Rect
// ============================================================================

TEST_CASE("Rect default is empty", "[screen]") {
    Rect r;
    REQUIRE(r.is_empty());
}

TEST_CASE("Rect expand from empty", "[screen]") {
    Rect r;
    r.expand(5, 10);
    REQUIRE_FALSE(r.is_empty());
    REQUIRE(r.x == 5);
    REQUIRE(r.y == 10);
    REQUIRE(r.width == 1);
    REQUIRE(r.height == 1);
}

TEST_CASE("Rect expand grows to encompass new point", "[screen]") {
    Rect r;
    r.expand(2, 2);  // becomes {2, 2, 1, 1}
    r.expand(5, 6);  // should grow to {2, 2, 4, 5}

    REQUIRE(r.x == 2);
    REQUIRE(r.y == 2);
    REQUIRE(r.width == 4);   // 5 - 2 + 1
    REQUIRE(r.height == 5);  // 6 - 2 + 1
}

TEST_CASE("Rect expand handles points before origin", "[screen]") {
    Rect r;
    r.expand(5, 5);
    r.expand(0, 0);

    REQUIRE(r.x == 0);
    REQUIRE(r.y == 0);
    REQUIRE(r.width == 6);
    REQUIRE(r.height == 6);
}

// ============================================================================
// Cursor
// ============================================================================

TEST_CASE("Cursor default is visible at origin", "[screen]") {
    Cursor c;
    REQUIRE(c.x == 0);
    REQUIRE(c.y == 0);
    REQUIRE(c.visible);
}

TEST_CASE("Cursor can be customized", "[screen]") {
    Cursor c{10, 5, false};
    REQUIRE(c.x == 10);
    REQUIRE(c.y == 5);
    REQUIRE_FALSE(c.visible);
}

// ============================================================================
// CellWidth
// ============================================================================

TEST_CASE("CellWidth enum values", "[screen]") {
    REQUIRE(static_cast<int>(CellWidth::Narrow) == 0);
    REQUIRE(static_cast<int>(CellWidth::Wide) == 1);
    REQUIRE(static_cast<int>(CellWidth::SpacerTail) == 2);
    REQUIRE(static_cast<int>(CellWidth::SpacerHead) == 3);
}

// ============================================================================
// STYLE_NONE constant
// ============================================================================

TEST_CASE("STYLE_NONE is zero", "[screen]") {
    REQUIRE(STYLE_NONE == 0);
}

// ============================================================================
// diff_screens
// ============================================================================

TEST_CASE("diff_screens detects changed cells", "[screen]") {
    Screen prev(10, 10);
    Screen next(10, 10);

    // Both screens start empty - no changes
    int change_count = 0;
    diff_screens(prev, next, [&](int x, int y, const Cell&, const Cell&, StyleId, StyleId) {
        (void)x; (void)y;
        change_count++;
    });
    REQUIRE(change_count == 0);

    // Modify next screen
    Cell c;
    c.codepoint = U'H';
    c.style = 5;
    next.set_cell(2, 3, c);

    change_count = 0;
    int last_x = -1, last_y = -1;
    diff_screens(prev, next, [&](int x, int y, const Cell& prev_cell, const Cell& next_cell, StyleId prev_style, StyleId next_style) {
        last_x = x;
        last_y = y;
        REQUIRE(prev_cell.codepoint == U' ');
        REQUIRE(next_cell.codepoint == U'H');
        REQUIRE(prev_style == STYLE_NONE);
        REQUIRE(next_style == 5);
        change_count++;
    });
    REQUIRE(change_count == 1);
    REQUIRE(last_x == 2);
    REQUIRE(last_y == 3);

    // Change another cell with different style only
    Cell c2;
    c2.codepoint = U' '; // same character
    c2.style = 99;       // different style
    next.set_cell(5, 5, c2);

    change_count = 0;
    diff_screens(prev, next, [&](int x, int y, const Cell& prev_cell, const Cell& next_cell, StyleId prev_style, StyleId next_style) {
        (void)x; (void)y;
        REQUIRE(prev_style != next_style);
        change_count++;
    });
    REQUIRE(change_count == 2); // both cells differ from prev
}

TEST_CASE("diff_screens detects width changes", "[screen]") {
    Screen prev(10, 10);
    Screen next(10, 10);

    Cell narrow;
    narrow.codepoint = U'A';
    narrow.width = CellWidth::Narrow;
    prev.set_cell(0, 0, narrow);

    Cell wide;
    wide.codepoint = U'A';
    wide.width = CellWidth::Wide;
    next.set_cell(0, 0, wide);

    // Same codepoint but different width should still trigger diff
    int change_count = 0;
    diff_screens(prev, next, [&](int, int, const Cell&, const Cell&, StyleId, StyleId) {
        change_count++;
    });
    REQUIRE(change_count == 1);
}

TEST_CASE("diff_screens handles different screen sizes", "[screen]") {
    Screen prev(5, 5);
    Screen next(10, 10);

    // Both empty - no changes in overlap region
    int change_count = 0;
    diff_screens(prev, next, [&](int, int, const Cell&, const Cell&, StyleId, StyleId) {
        change_count++;
    });
    REQUIRE(change_count == 0);

    // Put something in the overlap region
    Cell c;
    c.codepoint = U'X';
    next.set_cell(2, 2, c);

    change_count = 0;
    diff_screens(prev, next, [&](int, int, const Cell&, const Cell&, StyleId, StyleId) {
        change_count++;
    });
    REQUIRE(change_count == 1);

    // Put something outside the overlap region of prev
    next.set_cell(7, 7, c);

    change_count = 0;
    diff_screens(prev, next, [&](int, int, const Cell&, const Cell&, StyleId, StyleId) {
        change_count++;
    });
    // Only the cell at (2,2) should be reported, (7,7) is outside prev's bounds
    REQUIRE(change_count == 1);
}

TEST_CASE("diff_screens with multiple changes", "[screen]") {
    Screen prev(5, 5);
    Screen next(5, 5);

    Cell c;
    c.codepoint = U'A';
    next.set_cell(0, 0, c);
    c.codepoint = U'B';
    next.set_cell(1, 0, c);
    c.codepoint = U'C';
    next.set_cell(2, 0, c);

    int change_count = 0;
    diff_screens(prev, next, [&](int, int, const Cell&, const Cell&, StyleId, StyleId) {
        change_count++;
    });
    REQUIRE(change_count == 3);
}
