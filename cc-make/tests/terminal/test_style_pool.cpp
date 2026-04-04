#include <catch2/catch_test_macros.hpp>
#include "terminal/style_pool.hpp"

using namespace ccmake;

// ============================================================================
// Basic interning
// ============================================================================

TEST_CASE("interning a plain style returns a valid ID", "[style_pool]") {
    StylePool pool;
    TextStyle plain;
    uint32_t id = pool.intern(plain);
    REQUIRE(id == 0);  // plain style should be the first interned (ID 0)
    REQUIRE(pool.size() == 1);
}

TEST_CASE("same style returns same ID (dedup)", "[style_pool]") {
    StylePool pool;
    TextStyle style1;
    style1.bold = true;

    uint32_t id1 = pool.intern(style1);
    uint32_t id2 = pool.intern(style1);  // same style again

    REQUIRE(id1 == id2);
    REQUIRE(pool.size() == 1);
}

TEST_CASE("different styles return different IDs", "[style_pool]") {
    StylePool pool;
    TextStyle plain;
    TextStyle bold_style;
    bold_style.bold = true;
    TextStyle red_style;
    red_style.color = Color::rgb(255, 0, 0);

    uint32_t id_plain = pool.intern(plain);
    uint32_t id_bold = pool.intern(bold_style);
    uint32_t id_red = pool.intern(red_style);

    REQUIRE(id_plain != id_bold);
    REQUIRE(id_plain != id_red);
    REQUIRE(id_bold != id_red);
    REQUIRE(pool.size() == 3);
}

TEST_CASE("get returns the correct style for an ID", "[style_pool]") {
    StylePool pool;
    TextStyle style;
    style.bold = true;
    style.color = Color::ansi16(2); // green

    uint32_t id = pool.intern(style);
    const TextStyle& retrieved = pool.get(id);

    REQUIRE(retrieved.bold);
    REQUIRE(retrieved.color.has_value());
    REQUIRE(retrieved.color->type == ColorType::Ansi16);
    REQUIRE(retrieved.color->r == 2);
}

// ============================================================================
// none()
// ============================================================================

TEST_CASE("none() returns 0", "[style_pool]") {
    StylePool pool;
    REQUIRE(pool.none() == 0);
}

TEST_CASE("first interned plain style has ID 0 which equals none()", "[style_pool]") {
    StylePool pool;
    TextStyle plain;
    uint32_t id = pool.intern(plain);
    REQUIRE(id == pool.none());
}

// ============================================================================
// Interning with boolean attributes
// ============================================================================

TEST_CASE("each boolean attribute produces unique ID", "[style_pool]") {
    StylePool pool;

    TextStyle bold_s; bold_s.bold = true;
    TextStyle dim_s; dim_s.dim = true;
    TextStyle italic_s; italic_s.italic = true;
    TextStyle underline_s; underline_s.underline = true;
    TextStyle strike_s; strike_s.strikethrough = true;
    TextStyle inverse_s; inverse_s.inverse = true;

    uint32_t id_b = pool.intern(bold_s);
    uint32_t id_d = pool.intern(dim_s);
    uint32_t id_i = pool.intern(italic_s);
    uint32_t id_u = pool.intern(underline_s);
    uint32_t id_s = pool.intern(strike_s);
    uint32_t id_inv = pool.intern(inverse_s);

    // All IDs should be unique
    REQUIRE(id_b != id_d);
    REQUIRE(id_d != id_i);
    REQUIRE(id_i != id_u);
    REQUIRE(id_u != id_s);
    REQUIRE(id_s != id_inv);
    REQUIRE(pool.size() == 6);
}

TEST_CASE("combination of attributes produces unique style", "[style_pool]") {
    StylePool pool;

    TextStyle bold;
    bold.bold = true;
    TextStyle bold_italic;
    bold_italic.bold = true;
    bold_italic.italic = true;

    uint32_t id_b = pool.intern(bold);
    uint32_t id_bi = pool.intern(bold_italic);

    REQUIRE(id_b != id_bi);
    REQUIRE(pool.size() == 2);
}

// ============================================================================
// Interning with colors
// ============================================================================

TEST_CASE("same color returns same ID", "[style_pool]") {
    StylePool pool;

    TextStyle s1;
    s1.color = Color::rgb(255, 0, 0);
    TextStyle s2;
    s2.color = Color::rgb(255, 0, 0);

    uint32_t id1 = pool.intern(s1);
    uint32_t id2 = pool.intern(s2);

    REQUIRE(id1 == id2);
    REQUIRE(pool.size() == 1);
}

TEST_CASE("different fg colors produce different IDs", "[style_pool]") {
    StylePool pool;

    TextStyle red;
    red.color = Color::ansi16(1);
    TextStyle blue;
    blue.color = Color::ansi16(4);

    uint32_t id_red = pool.intern(red);
    uint32_t id_blue = pool.intern(blue);

    REQUIRE(id_red != id_blue);
}

TEST_CASE("fg vs bg color difference", "[style_pool]") {
    StylePool pool;

    TextStyle fg_red;
    fg_red.color = Color::ansi16(1);
    TextStyle bg_red;
    bg_red.bg_color = Color::ansi16(1);

    uint32_t id_fg = pool.intern(fg_red);
    uint32_t id_bg = pool.intern(bg_red);

    REQUIRE(id_fg != id_bg);
}

// ============================================================================
// Transitions
// ============================================================================

TEST_CASE("transition from none to bold returns bold SGR", "[style_pool]") {
    StylePool pool;
    TextStyle plain;
    TextStyle bold_style;
    bold_style.bold = true;

    uint32_t id_plain = pool.intern(plain);
    uint32_t id_bold = pool.intern(bold_style);

    std::string seq = pool.transition(id_plain, id_bold);
    REQUIRE(seq == "\x1b[1m");

    // Transition back should produce reset
    std::string seq_back = pool.transition(id_bold, id_plain);
    REQUIRE(seq_back == "\x1b[22m");

    // Same style transition returns empty
    std::string same = pool.transition(id_plain, id_plain);
    REQUIRE(same.empty());
}

TEST_CASE("transition from one color to another returns new color SGR", "[style_pool]") {
    StylePool pool;
    TextStyle red_style;
    red_style.color = Color::rgb(255, 0, 0);
    TextStyle blue_style;
    blue_style.color = Color::rgb(0, 0, 255);

    uint32_t id_red = pool.intern(red_style);
    uint32_t id_blue = pool.intern(blue_style);

    std::string seq = pool.transition(id_red, id_blue);
    // Should contain the new blue foreground SGR: "38;2;0;0;255"
    REQUIRE(seq.find("38;2;0;0;255") != std::string::npos);
    REQUIRE(seq.front() == '\x1b');
    REQUIRE(seq.back() == 'm');
}

TEST_CASE("transition removes color when going to none", "[style_pool]") {
    StylePool pool;
    TextStyle colored;
    colored.color = Color::ansi16(1); // red
    TextStyle plain;

    uint32_t id_colored = pool.intern(colored);
    uint32_t id_plain = pool.intern(plain);

    std::string seq = pool.transition(id_colored, id_plain);
    REQUIRE(seq == "\x1b[39m");
}

TEST_CASE("complex transition with multiple attributes", "[style_pool]") {
    StylePool pool;
    TextStyle plain;
    TextStyle complex;
    complex.bold = true;
    complex.italic = true;
    complex.color = Color::ansi16(2); // green
    complex.bg_color = Color::ansi16(0); // black

    uint32_t id_plain = pool.intern(plain);
    uint32_t id_complex = pool.intern(complex);

    std::string seq = pool.transition(id_plain, id_complex);
    // Should contain bold(1), italic(3), fg green(32), bg black(40)
    REQUIRE(seq.find("1") != std::string::npos);
    REQUIRE(seq.find("3") != std::string::npos);
    REQUIRE(seq.find("32") != std::string::npos);
    REQUIRE(seq.find("40") != std::string::npos);
}

TEST_CASE("transition with bg color change", "[style_pool]") {
    StylePool pool;
    TextStyle plain;
    TextStyle with_bg;
    with_bg.bg_color = Color::ansi16(1); // red bg

    uint32_t id_plain = pool.intern(plain);
    uint32_t id_bg = pool.intern(with_bg);

    std::string seq = pool.transition(id_plain, id_bg);
    REQUIRE(seq.find("41") != std::string::npos);  // bg red
}

TEST_CASE("transition removes bg color when going to none", "[style_pool]") {
    StylePool pool;
    TextStyle with_bg;
    with_bg.bg_color = Color::ansi16(1);
    TextStyle plain;

    uint32_t id_bg = pool.intern(with_bg);
    uint32_t id_plain = pool.intern(plain);

    std::string seq = pool.transition(id_bg, id_plain);
    REQUIRE(seq == "\x1b[49m");
}

TEST_CASE("transition with dim attribute", "[style_pool]") {
    StylePool pool;
    TextStyle plain;
    TextStyle dim_style;
    dim_style.dim = true;

    uint32_t id_plain = pool.intern(plain);
    uint32_t id_dim = pool.intern(dim_style);

    REQUIRE(pool.transition(id_plain, id_dim) == "\x1b[2m");
    REQUIRE(pool.transition(id_dim, id_plain) == "\x1b[22m");
}

TEST_CASE("transition with underline attribute", "[style_pool]") {
    StylePool pool;
    TextStyle plain;
    TextStyle ul;
    ul.underline = true;

    uint32_t id_plain = pool.intern(plain);
    uint32_t id_ul = pool.intern(ul);

    REQUIRE(pool.transition(id_plain, id_ul) == "\x1b[4m");
    REQUIRE(pool.transition(id_ul, id_plain) == "\x1b[24m");
}

TEST_CASE("transition with italic attribute", "[style_pool]") {
    StylePool pool;
    TextStyle plain;
    TextStyle it;
    it.italic = true;

    uint32_t id_plain = pool.intern(plain);
    uint32_t id_it = pool.intern(it);

    REQUIRE(pool.transition(id_plain, id_it) == "\x1b[3m");
    REQUIRE(pool.transition(id_it, id_plain) == "\x1b[23m");
}

TEST_CASE("transition with strikethrough attribute", "[style_pool]") {
    StylePool pool;
    TextStyle plain;
    TextStyle st;
    st.strikethrough = true;

    uint32_t id_plain = pool.intern(plain);
    uint32_t id_st = pool.intern(st);

    REQUIRE(pool.transition(id_plain, id_st) == "\x1b[9m");
    REQUIRE(pool.transition(id_st, id_plain) == "\x1b[29m");
}

TEST_CASE("transition with inverse attribute", "[style_pool]") {
    StylePool pool;
    TextStyle plain;
    TextStyle inv;
    inv.inverse = true;

    uint32_t id_plain = pool.intern(plain);
    uint32_t id_inv = pool.intern(inv);

    REQUIRE(pool.transition(id_plain, id_inv) == "\x1b[7m");
    REQUIRE(pool.transition(id_inv, id_plain) == "\x1b[27m");
}

TEST_CASE("transition with ansi256 color", "[style_pool]") {
    StylePool pool;
    TextStyle plain;
    TextStyle s256;
    s256.color = Color::ansi256(196);

    uint32_t id_plain = pool.intern(plain);
    uint32_t id_256 = pool.intern(s256);

    std::string seq = pool.transition(id_plain, id_256);
    REQUIRE(seq.find("38;5;196") != std::string::npos);
}

// ============================================================================
// Size tracking
// ============================================================================

TEST_CASE("size reflects number of unique interned styles", "[style_pool]") {
    StylePool pool;
    REQUIRE(pool.size() == 0);

    pool.intern(TextStyle{});
    REQUIRE(pool.size() == 1);

    TextStyle s;
    s.bold = true;
    pool.intern(s);
    REQUIRE(pool.size() == 2);

    // Duplicate should not increase size
    pool.intern(s);
    REQUIRE(pool.size() == 2);
}
