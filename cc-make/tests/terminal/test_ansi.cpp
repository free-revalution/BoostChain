#include <catch2/catch_test_macros.hpp>
#include "terminal/ansi.hpp"

using namespace ccmake::ansi;

TEST_CASE("cursor_up produces correct escape sequence", "[ansi]") {
    REQUIRE(cursor_up() == "\x1b[1A");
    REQUIRE(cursor_up(5) == "\x1b[5A");
}

TEST_CASE("cursor_to produces correct row;col sequence", "[ansi]") {
    REQUIRE(cursor_to(1, 1) == "\x1b[1;1H");
    REQUIRE(cursor_to(10, 20) == "\x1b[10;20H");
    REQUIRE(cursor_to(0, 0) == "\x1b[0;0H");
}

TEST_CASE("reset produces correct sequence", "[ansi]") {
    REQUIRE(reset() == "\x1b[0m");
}

TEST_CASE("fg_rgb produces correct 38;2;r;g;b sequence", "[ansi]") {
    REQUIRE(fg_rgb(255, 0, 128) == "\x1b[38;2;255;0;128m");
    REQUIRE(fg_rgb(0, 0, 0) == "\x1b[38;2;0;0;0m");
    REQUIRE(fg_rgb(10, 20, 30) == "\x1b[38;2;10;20;30m");
}

TEST_CASE("bg_256 produces correct 48;5;n sequence", "[ansi]") {
    REQUIRE(bg_256(0) == "\x1b[48;5;0m");
    REQUIRE(bg_256(196) == "\x1b[48;5;196m");
    REQUIRE(bg_256(255) == "\x1b[48;5;255m");
}

TEST_CASE("sgr produces correct multi-code sequence", "[ansi]") {
    REQUIRE(sgr({0}) == "\x1b[0m");
    REQUIRE(sgr({1, 31}) == "\x1b[1;31m");
    REQUIRE(sgr({1, 4, 32}) == "\x1b[1;4;32m");
    REQUIRE(sgr({38, 2, 255, 128, 0}) == "\x1b[38;2;255;128;0m");
}
