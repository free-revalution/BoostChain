#include <catch2/catch_test_macros.hpp>
#include "terminal/keybinding.hpp"
#include <nlohmann/json.hpp>

using namespace ccmake;

TEST_CASE("KeybindingManager loads defaults") {
    KeybindingManager mgr;
    REQUIRE(mgr.has_binding("enter"));
    REQUIRE(mgr.has_binding("return"));
    REQUIRE(mgr.has_binding("escape"));
    REQUIRE(mgr.has_binding("ctrl+c"));
    REQUIRE(mgr.has_binding("up"));
    REQUIRE(mgr.has_binding("down"));
}

TEST_CASE("KeybindingManager resolve Enter") {
    KeybindingManager mgr;
    ParsedKey key;
    key.name = "return";
    REQUIRE(mgr.resolve(key) == KeyAction::Submit);
}

TEST_CASE("KeybindingManager resolve Escape") {
    KeybindingManager mgr;
    ParsedKey key;
    key.name = "escape";
    REQUIRE(mgr.resolve(key) == KeyAction::Abort);
}

TEST_CASE("KeybindingManager resolve arrows") {
    KeybindingManager mgr;
    ParsedKey up; up.name = "up";
    ParsedKey down; down.name = "down";
    REQUIRE(mgr.resolve(up) == KeyAction::HistoryUp);
    REQUIRE(mgr.resolve(down) == KeyAction::HistoryDown);
}

TEST_CASE("KeybindingManager bind/unbind") {
    KeybindingManager mgr;
    mgr.bind("ctrl+x", KeyAction::Abort);
    REQUIRE(mgr.has_binding("ctrl+x"));
    mgr.unbind("enter");
    REQUIRE_FALSE(mgr.has_binding("enter"));
}

TEST_CASE("KeybindingManager resolve Ctrl+A") {
    KeybindingManager mgr;
    ParsedKey key;
    key.ctrl = true;
    key.name = "a";
    REQUIRE(mgr.resolve(key) == KeyAction::CursorHome);
}

TEST_CASE("KeybindingManager to_json") {
    KeybindingManager mgr;
    auto j = mgr.to_json();
    REQUIRE(j.is_object());
    REQUIRE(j["enter"] == "submit");
}

TEST_CASE("KeybindingManager resolve PageUp/PageDown") {
    KeybindingManager mgr;
    ParsedKey pu; pu.name = "pageup";
    ParsedKey pd; pd.name = "pagedown";
    REQUIRE(mgr.resolve(pu) == KeyAction::ScrollUp);
    REQUIRE(mgr.resolve(pd) == KeyAction::ScrollDown);
}
