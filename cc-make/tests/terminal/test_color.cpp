#include <catch2/catch_test_macros.hpp>
#include "terminal/color.hpp"

using namespace ccmake;

// ============================================================================
// Color factory methods
// ============================================================================

TEST_CASE("Color::rgb creates correct color", "[color]") {
    Color c = Color::rgb(255, 128, 0);
    REQUIRE(c.type == ColorType::RGB);
    REQUIRE(c.r == 255);
    REQUIRE(c.g == 128);
    REQUIRE(c.b == 0);
}

TEST_CASE("Color::ansi256 creates correct color", "[color]") {
    Color c = Color::ansi256(196);
    REQUIRE(c.type == ColorType::Ansi256);
    REQUIRE(c.r == 196);
    REQUIRE(c.g == 0);
    REQUIRE(c.b == 0);
}

TEST_CASE("Color::ansi16 creates correct color", "[color]") {
    Color c = Color::ansi16(3);
    REQUIRE(c.type == ColorType::Ansi16);
    REQUIRE(c.r == 3);
    REQUIRE(c.g == 0);
    REQUIRE(c.b == 0);
}

TEST_CASE("Color::hex parses 6-char hex string", "[color]") {
    Color c = Color::hex("#FF00FF");
    REQUIRE(c.type == ColorType::Hex);
    REQUIRE(c.r == 255);
    REQUIRE(c.g == 0);
    REQUIRE(c.b == 255);
}

TEST_CASE("Color::hex parses without leading hash", "[color]") {
    Color c = Color::hex("FF00FF");
    REQUIRE(c.type == ColorType::Hex);
    REQUIRE(c.r == 255);
    REQUIRE(c.g == 0);
    REQUIRE(c.b == 255);
}

TEST_CASE("Color::hex parses 3-char shorthand", "[color]") {
    Color c = Color::hex("#F0A");
    REQUIRE(c.type == ColorType::Hex);
    REQUIRE(c.r == 0xFF);
    REQUIRE(c.g == 0x00);
    REQUIRE(c.b == 0xAA);
}

TEST_CASE("Color::hex handles partial hex strings", "[color]") {
    Color c = Color::hex("AB");
    REQUIRE(c.type == ColorType::Hex);
    REQUIRE(c.r == 0xAB);
    REQUIRE(c.g == 0);
    REQUIRE(c.b == 0);
}

// ============================================================================
// Color::parse
// ============================================================================

TEST_CASE("Color::parse handles rgb() format", "[color]") {
    Color c = Color::parse("rgb(255, 128, 0)");
    REQUIRE(c.type == ColorType::RGB);
    REQUIRE(c.r == 255);
    REQUIRE(c.g == 128);
    REQUIRE(c.b == 0);
}

TEST_CASE("Color::parse handles rgb() with spaces", "[color]") {
    Color c = Color::parse("  rgb(255, 128, 0)  ");
    REQUIRE(c.type == ColorType::RGB);
    REQUIRE(c.r == 255);
    REQUIRE(c.g == 128);
    REQUIRE(c.b == 0);
}

TEST_CASE("Color::parse handles ansi256() format", "[color]") {
    Color c = Color::parse("ansi256(196)");
    REQUIRE(c.type == ColorType::Ansi256);
    REQUIRE(c.r == 196);
}

TEST_CASE("Color::parse handles hex color", "[color]") {
    Color c = Color::parse("#FF00FF");
    REQUIRE(c.type == ColorType::Hex);
    REQUIRE(c.r == 255);
    REQUIRE(c.g == 0);
    REQUIRE(c.b == 255);
}

TEST_CASE("Color::parse handles ansi: named colors", "[color]") {
    Color c = Color::parse("ansi:red");
    REQUIRE(c.type == ColorType::Ansi16);
    REQUIRE(c.r == 1);

    Color c2 = Color::parse("ansi:green");
    REQUIRE(c2.type == ColorType::Ansi16);
    REQUIRE(c2.r == 2);
}

TEST_CASE("Color::parse falls back to black for unknown input", "[color]") {
    Color c = Color::parse("unknown");
    REQUIRE(c.type == ColorType::RGB);
    REQUIRE(c.r == 0);
    REQUIRE(c.g == 0);
    REQUIRE(c.b == 0);
}

// ============================================================================
// Color SGR conversion
// ============================================================================

TEST_CASE("Color::to_sgr_fg for RGB", "[color]") {
    Color c = Color::rgb(255, 128, 0);
    REQUIRE(c.to_sgr_fg() == "38;2;255;128;0");
}

TEST_CASE("Color::to_sgr_fg for Ansi256", "[color]") {
    Color c = Color::ansi256(196);
    REQUIRE(c.to_sgr_fg() == "38;5;196");
}

TEST_CASE("Color::to_sgr_fg for Ansi16", "[color]") {
    Color c = Color::ansi16(1);  // red
    REQUIRE(c.to_sgr_fg() == "31");
    Color c2 = Color::ansi16(7);  // white
    REQUIRE(c2.to_sgr_fg() == "37");
}

TEST_CASE("Color::to_sgr_fg for Hex", "[color]") {
    Color c = Color::hex("#102030");
    REQUIRE(c.to_sgr_fg() == "38;2;16;32;48");
}

TEST_CASE("Color::to_sgr_bg for RGB", "[color]") {
    Color c = Color::rgb(0, 255, 128);
    REQUIRE(c.to_sgr_bg() == "48;2;0;255;128");
}

TEST_CASE("Color::to_sgr_bg for Ansi256", "[color]") {
    Color c = Color::ansi256(21);
    REQUIRE(c.to_sgr_bg() == "48;5;21");
}

TEST_CASE("Color::to_sgr_bg for Ansi16", "[color]") {
    Color c = Color::ansi16(1);  // red
    REQUIRE(c.to_sgr_bg() == "41");
    Color c2 = Color::ansi16(0);  // black
    REQUIRE(c2.to_sgr_bg() == "40");
}

TEST_CASE("Color::to_sgr_bg for Hex", "[color]") {
    Color c = Color::hex("#AABBCC");
    REQUIRE(c.to_sgr_bg() == "48;2;170;187;204");
}

// ============================================================================
// Color equality
// ============================================================================

TEST_CASE("Color equality works correctly", "[color]") {
    Color a = Color::rgb(255, 0, 0);
    Color b = Color::rgb(255, 0, 0);
    Color c = Color::rgb(0, 255, 0);
    Color d = Color::ansi256(255);
    Color e = Color::hex("#FF0000");

    REQUIRE(a == b);
    REQUIRE_FALSE(a == c);
    // Hex and RGB with same values are equal since both produce same type-independent comparison
    // Actually: Hex != RGB because type differs
    REQUIRE_FALSE(a == d);

    // hex and rgb with same component values should NOT be equal (different types)
    REQUIRE_FALSE(a == e);
}

TEST_CASE("Color equality for same type different values", "[color]") {
    Color a = Color::ansi256(0);
    Color b = Color::ansi256(1);
    REQUIRE_FALSE(a == b);
}

// ============================================================================
// TextStyle
// ============================================================================

TEST_CASE("default TextStyle is plain", "[color]") {
    TextStyle ts;
    REQUIRE(ts.is_plain());
    REQUIRE_FALSE(ts.bold);
    REQUIRE_FALSE(ts.dim);
    REQUIRE_FALSE(ts.italic);
    REQUIRE_FALSE(ts.underline);
    REQUIRE_FALSE(ts.strikethrough);
    REQUIRE_FALSE(ts.inverse);
    REQUIRE_FALSE(ts.color.has_value());
    REQUIRE_FALSE(ts.bg_color.has_value());
}

TEST_CASE("TextStyle with bold is not plain", "[color]") {
    TextStyle ts;
    ts.bold = true;
    REQUIRE_FALSE(ts.is_plain());
}

TEST_CASE("TextStyle with fg color is not plain", "[color]") {
    TextStyle ts;
    ts.color = Color::rgb(255, 0, 0);
    REQUIRE_FALSE(ts.is_plain());
}

TEST_CASE("TextStyle with bg color is not plain", "[color]") {
    TextStyle ts;
    ts.bg_color = Color::rgb(0, 0, 255);
    REQUIRE_FALSE(ts.is_plain());
}

TEST_CASE("TextStyle equality works", "[color]") {
    TextStyle a;
    TextStyle b;
    REQUIRE(a == b);

    a.bold = true;
    REQUIRE_FALSE(a == b);

    b.bold = true;
    REQUIRE(a == b);

    a.color = Color::rgb(255, 0, 0);
    REQUIRE_FALSE(a == b);

    b.color = Color::rgb(255, 0, 0);
    REQUIRE(a == b);
}

TEST_CASE("TextStyle equality for each boolean attribute", "[color]") {
    TextStyle a;
    TextStyle b;

    a.dim = true;  REQUIRE_FALSE(a == b); b.dim = true;  REQUIRE(a == b);
    a.italic = true; REQUIRE_FALSE(a == b); b.italic = true; REQUIRE(a == b);
    a.underline = true; REQUIRE_FALSE(a == b); b.underline = true; REQUIRE(a == b);
    a.strikethrough = true; REQUIRE_FALSE(a == b); b.strikethrough = true; REQUIRE(a == b);
    a.inverse = true; REQUIRE_FALSE(a == b); b.inverse = true; REQUIRE(a == b);
}

// ============================================================================
// parse_ansi_color_name
// ============================================================================

TEST_CASE("parse_ansi_color_name for standard colors", "[color]") {
    REQUIRE(parse_ansi_color_name("black") == 0);
    REQUIRE(parse_ansi_color_name("red") == 1);
    REQUIRE(parse_ansi_color_name("green") == 2);
    REQUIRE(parse_ansi_color_name("yellow") == 3);
    REQUIRE(parse_ansi_color_name("blue") == 4);
    REQUIRE(parse_ansi_color_name("magenta") == 5);
    REQUIRE(parse_ansi_color_name("cyan") == 6);
    REQUIRE(parse_ansi_color_name("white") == 7);
}

TEST_CASE("parse_ansi_color_name for bright colors (XxxBright form)", "[color]") {
    REQUIRE(parse_ansi_color_name("blackBright") == 8);
    REQUIRE(parse_ansi_color_name("redBright") == 9);
    REQUIRE(parse_ansi_color_name("greenBright") == 10);
    REQUIRE(parse_ansi_color_name("yellowBright") == 11);
    REQUIRE(parse_ansi_color_name("blueBright") == 12);
    REQUIRE(parse_ansi_color_name("magentaBright") == 13);
    REQUIRE(parse_ansi_color_name("cyanBright") == 14);
    REQUIRE(parse_ansi_color_name("whiteBright") == 15);
}

TEST_CASE("parse_ansi_color_name for bright colors (brightXxx form)", "[color]") {
    REQUIRE(parse_ansi_color_name("brightBlack") == 8);
    REQUIRE(parse_ansi_color_name("brightRed") == 9);
    REQUIRE(parse_ansi_color_name("brightGreen") == 10);
    REQUIRE(parse_ansi_color_name("brightYellow") == 11);
    REQUIRE(parse_ansi_color_name("brightBlue") == 12);
    REQUIRE(parse_ansi_color_name("brightMagenta") == 13);
    REQUIRE(parse_ansi_color_name("brightCyan") == 14);
    REQUIRE(parse_ansi_color_name("brightWhite") == 15);
}

TEST_CASE("parse_ansi_color_name returns nullopt for unknown names", "[color]") {
    REQUIRE_FALSE(parse_ansi_color_name("unknown").has_value());
    REQUIRE_FALSE(parse_ansi_color_name("").has_value());
    REQUIRE_FALSE(parse_ansi_color_name("RED").has_value());  // case sensitive
}
