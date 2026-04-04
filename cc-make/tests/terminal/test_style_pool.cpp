#include <catch2/catch_test_macros.hpp>
#include "terminal/style_pool.hpp"

using namespace ccmake;

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

TEST_CASE("transition from one color to another returns reset+apply", "[style_pool]") {
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
