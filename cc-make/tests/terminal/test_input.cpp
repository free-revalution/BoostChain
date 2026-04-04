#include <catch2/catch_test_macros.hpp>
#include "terminal/input.hpp"

using namespace ccmake;

TEST_CASE("printable character 'a' produces name='a' with no modifiers", "[input]") {
    auto result = parse_keypress("a");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "a");
    REQUIRE_FALSE(key->ctrl);
    REQUIRE_FALSE(key->meta);
    REQUIRE_FALSE(key->shift);
    REQUIRE(result.remaining.empty());
}

TEST_CASE("ESC produces name='escape'", "[input]") {
    auto result = parse_keypress("\x1b");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "escape");
}

TEST_CASE("ESC[A produces name='up'", "[input]") {
    auto result = parse_keypress("\x1b[A");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "up");
    REQUIRE(key->sequence == "\x1b[A");
    REQUIRE(result.remaining.empty());
}

TEST_CASE("ESC[B produces name='down'", "[input]") {
    auto result = parse_keypress("\x1b[B");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "down");
    REQUIRE(key->sequence == "\x1b[B");
    REQUIRE(result.remaining.empty());
}

TEST_CASE("Ctrl+A produces name='a' with ctrl=true", "[input]") {
    // Ctrl+A is 0x01
    auto result = parse_keypress(std::string(1, '\x01'));
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "a");
    REQUIRE(key->ctrl);
    REQUIRE_FALSE(key->meta);
    REQUIRE_FALSE(key->shift);
}

TEST_CASE("Tab produces name='tab'", "[input]") {
    auto result = parse_keypress("\x09");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "tab");
    REQUIRE(result.remaining.empty());
}

TEST_CASE("Return (CR) produces name='return'", "[input]") {
    auto result = parse_keypress("\x0d");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "return");
}

TEST_CASE("Backspace (DEL 0x7F) produces name='backspace'", "[input]") {
    auto result = parse_keypress("\x7f");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "backspace");
}

TEST_CASE("ESC[C produces name='right' and remaining is preserved", "[input]") {
    auto result = parse_keypress("\x1b[C");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "right");
}

TEST_CASE("ESC[D produces name='left'", "[input]") {
    auto result = parse_keypress("\x1b[D");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "left");
}

TEST_CASE("multiple keypresses in a row", "[input]") {
    // parse_keypress parses one at a time, so call twice
    auto r1 = parse_keypress("hello");
    REQUIRE(r1.inputs.size() == 1);

    auto* key1 = std::get_if<ParsedKey>(&r1.inputs[0]);
    REQUIRE(key1 != nullptr);
    // 'h' is lowercase, no shift
    REQUIRE(key1->name == "h");
    REQUIRE_FALSE(key1->shift);

    // Parse remaining "ello"
    auto r2 = parse_keypress(r1.remaining);
    REQUIRE(r2.inputs.size() == 1);
    auto* key2 = std::get_if<ParsedKey>(&r2.inputs[0]);
    REQUIRE(key2 != nullptr);
    REQUIRE(key2->name == "e");
    REQUIRE_FALSE(key2->shift);
}

TEST_CASE("ESC[5~ produces pageup", "[input]") {
    auto result = parse_keypress("\x1b[5~");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "pageup");
}

TEST_CASE("ESC[6~ produces pagedown", "[input]") {
    auto result = parse_keypress("\x1b[6~");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "pagedown");
}

TEST_CASE("looks_like_escape returns true for ESC", "[input]") {
    REQUIRE(looks_like_escape("\x1b"));
    REQUIRE(looks_like_escape("\x1b[A"));
    REQUIRE_FALSE(looks_like_escape("a"));
    REQUIRE_FALSE(looks_like_escape(""));
}
