#include <catch2/catch_test_macros.hpp>
#include "ui/prompt_input.hpp"

using namespace ccmake;

TEST_CASE("PromptInput initial state") {
    PromptInput input;
    REQUIRE(input.text().empty());
    REQUIRE_FALSE(input.is_multiline());
    REQUIRE(input.cursor_col() == 0);
    REQUIRE(input.cursor_row() == 0);
}

TEST_CASE("PromptInput insert_char") {
    PromptInput input;
    ParsedKey key;
    key.name = "h";
    input.handle_key(key);
    REQUIRE(input.text() == "h");
    REQUIRE(input.cursor_col() == 1);
}

TEST_CASE("PromptInput multiple chars") {
    PromptInput input;
    for (char c : {'h', 'e', 'l', 'l', 'o'}) {
        ParsedKey key;
        key.name = std::string(1, c);
        input.handle_key(key);
    }
    REQUIRE(input.text() == "hello");
    REQUIRE(input.cursor_col() == 5);
}

TEST_CASE("PromptInput backspace") {
    PromptInput input;
    ParsedKey h, e, bs;
    h.name = "h"; input.handle_key(h);
    e.name = "e"; input.handle_key(e);
    bs.name = "backspace"; input.handle_key(bs);
    REQUIRE(input.text() == "h");
    REQUIRE(input.cursor_col() == 1);
}

TEST_CASE("PromptInput backspace at start") {
    PromptInput input;
    ParsedKey bs;
    bs.name = "backspace";
    input.handle_key(bs);
    REQUIRE(input.text().empty());
}

TEST_CASE("PromptInput history") {
    PromptInput input;
    input.push_history("hello");
    input.push_history("world");

    ParsedKey up, down;
    up.name = "up";
    down.name = "down";

    input.history_up();
    REQUIRE(input.text() == "world");

    input.history_up();
    REQUIRE(input.text() == "hello");

    input.history_down();
    REQUIRE(input.text() == "world");

    input.history_down();
    REQUIRE(input.text().empty());
}

TEST_CASE("PromptInput clear") {
    PromptInput input;
    ParsedKey k;
    k.name = "x";
    input.handle_key(k);
    input.clear();
    REQUIRE(input.text().empty());
}

TEST_CASE("PromptInput multiline") {
    PromptInput input;
    input.enter_multiline();

    ParsedKey h, nl, w;
    h.name = "h"; input.handle_key(h);
    nl.name = "return"; input.handle_key(nl);
    w.name = "w"; input.handle_key(w);

    REQUIRE(input.is_multiline());
    REQUIRE(input.text() == "h\nw");
    REQUIRE(input.cursor_row() == 1);
}

TEST_CASE("PromptInput kill_to_start") {
    PromptInput input;
    ParsedKey a, b, c;
    a.name = "a"; input.handle_key(a);
    b.name = "b"; input.handle_key(b);
    c.name = "c"; input.handle_key(c);

    input.kill_to_start();
    REQUIRE(input.text().empty());
}

TEST_CASE("PromptInput kill_to_end") {
    PromptInput input;
    ParsedKey a, b, c;
    a.name = "a"; input.handle_key(a);
    b.name = "b"; input.handle_key(b);
    c.name = "c"; input.handle_key(c);

    // Move cursor to position 1 using left arrow
    ParsedKey left; left.name = "left";
    input.handle_key(left);
    input.handle_key(left);
    input.kill_to_end();
    REQUIRE(input.text() == "a");
}

TEST_CASE("PromptInput yank") {
    PromptInput input;
    ParsedKey a, b;
    a.name = "a"; input.handle_key(a);
    b.name = "b"; input.handle_key(b);

    input.kill_to_start();
    input.yank();
    REQUIRE(input.text() == "ab");
}
