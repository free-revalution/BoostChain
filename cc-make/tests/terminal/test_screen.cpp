#include <catch2/catch_test_macros.hpp>
#include "terminal/screen.hpp"

using namespace ccmake;

TEST_CASE("Screen creation has correct dimensions", "[screen]") {
    Screen s(80, 24);
    REQUIRE(s.width() == 80);
    REQUIRE(s.height() == 24);
    REQUIRE(s.in_bounds(0, 0));
    REQUIRE(s.in_bounds(79, 23));
    REQUIRE_FALSE(s.in_bounds(80, 24));
    REQUIRE_FALSE(s.in_bounds(-1, 0));
}

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
