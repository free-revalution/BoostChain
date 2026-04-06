#include <catch2/catch_test_macros.hpp>
#include "terminal/vim_mode.hpp"

using namespace ccmake;

TEST_CASE("VimModeHandler starts in Insert mode") {
    VimModeHandler vim;
    REQUIRE(vim.mode() == VimMode::Insert);
}

TEST_CASE("VimModeHandler Escape enters Normal") {
    VimModeHandler vim;
    ParsedKey key;
    key.name = "escape";
    vim.process_key(key);
    REQUIRE(vim.mode() == VimMode::Normal);
}

TEST_CASE("VimModeHandler i enters Insert") {
    VimModeHandler vim;
    vim.enter_normal();
    ParsedKey key;
    key.name = "i";
    vim.process_key(key);
    REQUIRE(vim.mode() == VimMode::Insert);
}

TEST_CASE("VimModeHandler a enters Insert") {
    VimModeHandler vim;
    vim.enter_normal();
    ParsedKey key;
    key.name = "a";
    auto action = vim.process_key(key);
    REQUIRE(vim.mode() == VimMode::Insert);
    REQUIRE(action.has_value());
    REQUIRE(*action == KeyAction::CursorRight);
}

TEST_CASE("VimModeHandler h/j/k/l motions") {
    VimModeHandler vim;
    vim.enter_normal();
    ParsedKey h; h.name = "h";
    auto r1 = vim.process_key(h);
    REQUIRE(r1.has_value());
    REQUIRE(*r1 == KeyAction::CursorLeft);

    ParsedKey j; j.name = "j";
    vim.process_key(j);

    ParsedKey l; l.name = "l";
    auto r3 = vim.process_key(l);
    REQUIRE(r3.has_value());
    REQUIRE(*r3 == KeyAction::CursorRight);
}

TEST_CASE("VimModeHandler dd") {
    VimModeHandler vim;
    vim.enter_normal();
    ParsedKey d; d.name = "d";
    vim.process_key(d);
    auto result = vim.process_key(d);
    REQUIRE(result.has_value());
}

TEST_CASE("VimModeHandler cc enters Insert") {
    VimModeHandler vim;
    vim.enter_normal();
    ParsedKey c; c.name = "c";
    vim.process_key(c);
    auto result = vim.process_key(c);
    REQUIRE(result.has_value());
    REQUIRE(vim.mode() == VimMode::Insert);
}

TEST_CASE("VimModeHandler w/b motions") {
    VimModeHandler vim;
    vim.enter_normal();
    ParsedKey w; w.name = "w";
    auto r1 = vim.process_key(w);
    REQUIRE(r1.has_value());

    ParsedKey b; b.name = "b";
    auto r2 = vim.process_key(b);
    REQUIRE(r2.has_value());
}

TEST_CASE("VimModeHandler 0/$ motions") {
    VimModeHandler vim;
    vim.enter_normal();
    ParsedKey zero; zero.name = "0";
    auto r1 = vim.process_key(zero);
    REQUIRE(r1.has_value());
    REQUIRE(*r1 == KeyAction::CursorHome);

    ParsedKey dollar; dollar.name = "$";
    auto r2 = vim.process_key(dollar);
    REQUIRE(r2.has_value());
    REQUIRE(*r2 == KeyAction::CursorEnd);
}

TEST_CASE("VimModeHandler G/gg") {
    VimModeHandler vim;
    vim.enter_normal();
    ParsedKey g; g.name = "G";
    auto r1 = vim.process_key(g);
    REQUIRE(r1.has_value());
    REQUIRE(*r1 == KeyAction::ScrollToBottom);

    vim.enter_normal();
    ParsedKey g1; g1.name = "g";
    vim.process_key(g1);
    ParsedKey g2; g2.name = "g";
    auto r2 = vim.process_key(g2);
    REQUIRE(r2.has_value());
    REQUIRE(*r2 == KeyAction::ScrollToTop);
}

TEST_CASE("VimModeHandler insert passes regular chars") {
    VimModeHandler vim;
    ParsedKey key;
    key.name = "x";
    auto result = vim.process_key(key);
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("VimModeHandler count prefix") {
    VimModeHandler vim;
    vim.enter_normal();
    ParsedKey three; three.name = "3";
    vim.process_key(three);
    ParsedKey d; d.name = "d";
    vim.process_key(d);  // Sets pending operator
    ParsedKey d2; d2.name = "d";
    auto result = vim.process_key(d2);  // dd with count
    REQUIRE(result.has_value());
}
