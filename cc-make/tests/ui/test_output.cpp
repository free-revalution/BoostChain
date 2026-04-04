#include <catch2/catch_test_macros.hpp>
#include "ui/output.hpp"
#include "terminal/style_pool.hpp"

using namespace ccmake;

// Helper: create a plain StylePool with a plain style interned as id=0
static StylePool make_pool() {
    StylePool pool;
    TextStyle plain;
    pool.intern(plain);
    return pool;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_CASE("Output construction creates screen with correct dimensions", "[output]") {
    StylePool pool = make_pool();
    Output output(40, 10, pool);

    REQUIRE(output.screen().width() == 40);
    REQUIRE(output.screen().height() == 10);
}

TEST_CASE("Output screen cells are initialized to defaults", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 10; ++x) {
            REQUIRE(output.screen().cell_at(x, y).codepoint == U' ');
            REQUIRE(output.screen().cell_at(x, y).style == STYLE_NONE);
        }
    }
}

// ---------------------------------------------------------------------------
// Write single character
// ---------------------------------------------------------------------------

TEST_CASE("Output write single character at position", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(3, 2, "A", 0);

    REQUIRE(output.screen().cell_at(3, 2).codepoint == U'A');
}

TEST_CASE("Output write single character does not affect other cells", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(3, 2, "A", 0);

    // Adjacent cells should be unchanged
    REQUIRE(output.screen().cell_at(2, 2).codepoint == U' ');
    REQUIRE(output.screen().cell_at(4, 2).codepoint == U' ');
    REQUIRE(output.screen().cell_at(3, 1).codepoint == U' ');
    REQUIRE(output.screen().cell_at(3, 3).codepoint == U' ');
}

// ---------------------------------------------------------------------------
// Write string
// ---------------------------------------------------------------------------

TEST_CASE("Output write text places characters on screen", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(0, 0, "hello", 0);

    REQUIRE(output.screen().cell_at(0, 0).codepoint == U'h');
    REQUIRE(output.screen().cell_at(1, 0).codepoint == U'e');
    REQUIRE(output.screen().cell_at(2, 0).codepoint == U'l');
    REQUIRE(output.screen().cell_at(3, 0).codepoint == U'l');
    REQUIRE(output.screen().cell_at(4, 0).codepoint == U'o');
}

TEST_CASE("Output write string at non-origin position", "[output]") {
    StylePool pool = make_pool();
    Output output(20, 10, pool);

    output.write(5, 3, "test", 0);

    REQUIRE(output.screen().cell_at(5, 3).codepoint == U't');
    REQUIRE(output.screen().cell_at(6, 3).codepoint == U'e');
    REQUIRE(output.screen().cell_at(7, 3).codepoint == U's');
    REQUIRE(output.screen().cell_at(8, 3).codepoint == U't');
    // Cells before and after should be spaces
    REQUIRE(output.screen().cell_at(4, 3).codepoint == U' ');
    REQUIRE(output.screen().cell_at(9, 3).codepoint == U' ');
}

// ---------------------------------------------------------------------------
// Write with style ID
// ---------------------------------------------------------------------------

TEST_CASE("Output write with style ID sets cell style", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(0, 0, "X", 42);

    REQUIRE(output.screen().cell_at(0, 0).codepoint == U'X');
    REQUIRE(output.screen().cell_at(0, 0).style == 42);
}

TEST_CASE("Output write with different styles per character position", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(0, 0, "A", 1);
    output.write(1, 0, "B", 2);
    output.write(2, 0, "C", 3);

    REQUIRE(output.screen().cell_at(0, 0).style == 1);
    REQUIRE(output.screen().cell_at(1, 0).style == 2);
    REQUIRE(output.screen().cell_at(2, 0).style == 3);
}

// ---------------------------------------------------------------------------
// Newline handling
// ---------------------------------------------------------------------------

TEST_CASE("Output write newline resets x and advances y", "[output]") {
    StylePool pool = make_pool();
    Output output(20, 10, pool);

    output.write(5, 2, "AB\nCD", 0);

    REQUIRE(output.screen().cell_at(5, 2).codepoint == U'A');
    REQUIRE(output.screen().cell_at(6, 2).codepoint == U'B');
    // After newline, x resets to original position (5), y advances to 3
    REQUIRE(output.screen().cell_at(5, 3).codepoint == U'C');
    REQUIRE(output.screen().cell_at(6, 3).codepoint == U'D');
}

TEST_CASE("Output write multiple newlines", "[output]") {
    StylePool pool = make_pool();
    Output output(20, 10, pool);

    output.write(3, 1, "X\nY\nZ", 0);

    REQUIRE(output.screen().cell_at(3, 1).codepoint == U'X');
    REQUIRE(output.screen().cell_at(3, 2).codepoint == U'Y');
    REQUIRE(output.screen().cell_at(3, 3).codepoint == U'Z');
}

TEST_CASE("Output write text after newline continues at original x", "[output]") {
    StylePool pool = make_pool();
    Output output(20, 10, pool);

    output.write(4, 0, "foo\nbar", 0);

    // "foo" at y=0, "bar" at y=1, both starting at x=4
    REQUIRE(output.screen().cell_at(4, 0).codepoint == U'f');
    REQUIRE(output.screen().cell_at(5, 0).codepoint == U'o');
    REQUIRE(output.screen().cell_at(6, 0).codepoint == U'o');
    REQUIRE(output.screen().cell_at(4, 1).codepoint == U'b');
    REQUIRE(output.screen().cell_at(5, 1).codepoint == U'a');
    REQUIRE(output.screen().cell_at(6, 1).codepoint == U'r');
}

// ---------------------------------------------------------------------------
// Out-of-bounds write
// ---------------------------------------------------------------------------

TEST_CASE("Output write out of bounds x negative is ignored", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(-1, 0, "X", 0);

    REQUIRE(output.screen().cell_at(0, 0).codepoint == U' ');
}

TEST_CASE("Output write out of bounds x beyond width is ignored", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(10, 0, "X", 0);

    // Screen is only 10 wide (indices 0-9), so x=10 is out of bounds
    REQUIRE(output.screen().cell_at(9, 0).codepoint == U' ');
}

TEST_CASE("Output write out of bounds y negative is ignored", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(0, -1, "X", 0);

    REQUIRE(output.screen().cell_at(0, 0).codepoint == U' ');
}

TEST_CASE("Output write out of bounds y beyond height is ignored", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(0, 5, "X", 0);

    // Screen is only 5 tall (indices 0-4), so y=5 is out of bounds
    REQUIRE(output.screen().cell_at(0, 4).codepoint == U' ');
}

TEST_CASE("Output write partial string clips at screen boundary", "[output]") {
    StylePool pool = make_pool();
    Output output(5, 3, pool);

    // Write a long string that extends beyond screen width
    output.write(3, 0, "ABCDEFGH", 0);

    // Only "AB" should fit at positions 3 and 4
    REQUIRE(output.screen().cell_at(3, 0).codepoint == U'A');
    REQUIRE(output.screen().cell_at(4, 0).codepoint == U'B');
    // Position 5 is out of bounds (screen width = 5), nothing more written
}

// ---------------------------------------------------------------------------
// Clear region
// ---------------------------------------------------------------------------

TEST_CASE("Output clear region resets cells to defaults", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(0, 0, "hello", 0);
    REQUIRE(output.screen().cell_at(2, 0).codepoint == U'l');

    output.clear(0, 0, 5, 1);

    REQUIRE(output.screen().cell_at(0, 0).codepoint == U' ');
    REQUIRE(output.screen().cell_at(1, 0).codepoint == U' ');
    REQUIRE(output.screen().cell_at(2, 0).codepoint == U' ');
    REQUIRE(output.screen().cell_at(3, 0).codepoint == U' ');
    REQUIRE(output.screen().cell_at(4, 0).codepoint == U' ');
}

TEST_CASE("Output clear region resets styles to none", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(0, 0, "X", 42);
    REQUIRE(output.screen().cell_at(0, 0).style == 42);

    output.clear(0, 0, 1, 1);

    REQUIRE(output.screen().cell_at(0, 0).style == STYLE_NONE);
}

TEST_CASE("Output clear region preserves cells outside the region", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(0, 0, "ABCDEFGH", 0);
    output.clear(2, 0, 3, 1);  // Clear columns 2-4

    REQUIRE(output.screen().cell_at(0, 0).codepoint == U'A');
    REQUIRE(output.screen().cell_at(1, 0).codepoint == U'B');
    REQUIRE(output.screen().cell_at(2, 0).codepoint == U' ');
    REQUIRE(output.screen().cell_at(3, 0).codepoint == U' ');
    REQUIRE(output.screen().cell_at(4, 0).codepoint == U' ');
    REQUIRE(output.screen().cell_at(5, 0).codepoint == U'F');
    REQUIRE(output.screen().cell_at(6, 0).codepoint == U'G');
    REQUIRE(output.screen().cell_at(7, 0).codepoint == U'H');
}

TEST_CASE("Output clear multi-line region", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    // Fill the entire screen with characters
    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 10; ++x) {
            output.write(x, y, "X", 0);
        }
    }

    // Clear a 3x2 region
    output.clear(2, 1, 3, 2);

    // Inside the cleared region should be spaces
    REQUIRE(output.screen().cell_at(2, 1).codepoint == U' ');
    REQUIRE(output.screen().cell_at(3, 1).codepoint == U' ');
    REQUIRE(output.screen().cell_at(4, 1).codepoint == U' ');
    REQUIRE(output.screen().cell_at(2, 2).codepoint == U' ');
    REQUIRE(output.screen().cell_at(3, 2).codepoint == U' ');
    REQUIRE(output.screen().cell_at(4, 2).codepoint == U' ');

    // Outside should still be 'X'
    REQUIRE(output.screen().cell_at(1, 1).codepoint == U'X');
    REQUIRE(output.screen().cell_at(5, 1).codepoint == U'X');
    REQUIRE(output.screen().cell_at(2, 0).codepoint == U'X');
    REQUIRE(output.screen().cell_at(2, 3).codepoint == U'X');
}

// ---------------------------------------------------------------------------
// Multiple writes to same position (overwrite)
// ---------------------------------------------------------------------------

TEST_CASE("Output overwrite same position replaces character", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(0, 0, "A", 0);
    REQUIRE(output.screen().cell_at(0, 0).codepoint == U'A');

    output.write(0, 0, "B", 0);
    REQUIRE(output.screen().cell_at(0, 0).codepoint == U'B');
}

TEST_CASE("Output overwrite same position replaces style", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(0, 0, "X", 1);
    REQUIRE(output.screen().cell_at(0, 0).style == 1);

    output.write(0, 0, "X", 99);
    REQUIRE(output.screen().cell_at(0, 0).style == 99);
}

TEST_CASE("Output overwrite partial overlap of string", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(0, 0, "AAAAAA", 0);
    output.write(2, 0, "BB", 0);

    REQUIRE(output.screen().cell_at(0, 0).codepoint == U'A');
    REQUIRE(output.screen().cell_at(1, 0).codepoint == U'A');
    REQUIRE(output.screen().cell_at(2, 0).codepoint == U'B');
    REQUIRE(output.screen().cell_at(3, 0).codepoint == U'B');
    REQUIRE(output.screen().cell_at(4, 0).codepoint == U'A');
    REQUIRE(output.screen().cell_at(5, 0).codepoint == U'A');
}

// ---------------------------------------------------------------------------
// screen() returns internal screen buffer
// ---------------------------------------------------------------------------

TEST_CASE("Output screen returns mutable reference to internal buffer", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    // Modify through screen() reference
    Cell cell;
    cell.codepoint = U'Z';
    cell.style = 7;
    output.screen().set_cell(0, 0, cell);

    REQUIRE(output.screen().cell_at(0, 0).codepoint == U'Z');
    REQUIRE(output.screen().cell_at(0, 0).style == 7);
}

TEST_CASE("Output screen reference reflects changes after write", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    Screen& ref = output.screen();
    REQUIRE(ref.cell_at(0, 0).codepoint == U' ');

    output.write(0, 0, "K", 0);

    // The reference should reflect the change
    REQUIRE(ref.cell_at(0, 0).codepoint == U'K');
}

// ---------------------------------------------------------------------------
// set_screen() updates dimensions
// ---------------------------------------------------------------------------

TEST_CASE("Output set_screen updates width and height", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);
    REQUIRE(output.screen().width() == 10);
    REQUIRE(output.screen().height() == 5);

    Screen new_screen(20, 8);
    output.set_screen(new_screen);

    REQUIRE(output.screen().width() == 20);
    REQUIRE(output.screen().height() == 8);
}

TEST_CASE("Output set_screen clears previous content", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(0, 0, "hello", 0);
    REQUIRE(output.screen().cell_at(0, 0).codepoint == U'h');

    Screen new_screen(10, 5);
    output.set_screen(new_screen);

    // Previous content should be gone (new screen buffer)
    REQUIRE(output.screen().cell_at(0, 0).codepoint == U' ');
}

TEST_CASE("Output write after set_screen uses new dimensions", "[output]") {
    StylePool pool = make_pool();
    Output output(5, 3, pool);

    // Old screen was 5x3
    Screen new_screen(10, 10);
    output.set_screen(new_screen);

    // Now we can write at positions that would have been out of bounds before
    output.write(7, 8, "X", 0);
    REQUIRE(output.screen().cell_at(7, 8).codepoint == U'X');
}

// ---------------------------------------------------------------------------
// Empty writes
// ---------------------------------------------------------------------------

TEST_CASE("Output write empty string does nothing", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(3, 2, "", 0);

    REQUIRE(output.screen().cell_at(3, 2).codepoint == U' ');
}

TEST_CASE("Output clear zero-size region does nothing", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(0, 0, "X", 0);
    output.clear(0, 0, 0, 0);

    REQUIRE(output.screen().cell_at(0, 0).codepoint == U'X');
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST_CASE("Output write newline only does not write any character", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(0, 0, "\n", 0);

    // The newline should not place any character
    REQUIRE(output.screen().cell_at(0, 0).codepoint == U' ');
    REQUIRE(output.screen().cell_at(0, 1).codepoint == U' ');
}

TEST_CASE("Output write at exact screen boundary", "[output]") {
    StylePool pool = make_pool();
    Output output(5, 3, pool);

    // Write at the last valid position (width-1, height-1)
    output.write(4, 2, "Z", 0);

    REQUIRE(output.screen().cell_at(4, 2).codepoint == U'Z');
}

TEST_CASE("Output write with style zero matches STYLE_NONE constant", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(0, 0, "A", 0);

    REQUIRE(output.screen().cell_at(0, 0).style == STYLE_NONE);
}
