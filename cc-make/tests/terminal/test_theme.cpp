#include <catch2/catch_test_macros.hpp>
#include "terminal/theme.hpp"
#include <nlohmann/json.hpp>

using namespace ccmake;

TEST_CASE("Theme::dark creates valid dark theme") {
    auto theme = Theme::dark();
    REQUIRE(theme.name == "dark");
    REQUIRE(theme.use_colors);
}

TEST_CASE("Theme::light creates valid light theme") {
    auto theme = Theme::light();
    REQUIRE(theme.name == "light");
}

TEST_CASE("Theme::monokai creates valid monokai theme") {
    auto theme = Theme::monokai();
    REQUIRE(theme.name == "monokai");
    REQUIRE(theme.colors.background.r < 50);
    REQUIRE(theme.colors.primary_text.r > 200);
}

TEST_CASE("Theme::to_json serializes theme") {
    auto theme = Theme::dark();
    auto j = theme.to_json();
    REQUIRE(j["name"] == "dark");
    REQUIRE(j["use_colors"] == true);
    REQUIRE(j["colors"].contains("primary_text"));
    REQUIRE(j["colors"].contains("background"));
}

TEST_CASE("Theme::from_json deserializes theme") {
    nlohmann::json j = {
        {"name", "custom"},
        {"use_colors", true},
        {"colors", {
            {"primary_text", "#ff0000"},
            {"background", "#000000"},
            {"accent", "#00ff00"}
        }}
    };
    auto theme = Theme::from_json(j);
    REQUIRE(theme.name == "custom");
    REQUIRE(theme.colors.primary_text.r == 255);
    REQUIRE(theme.colors.background.r == 0);
}

TEST_CASE("ThemeManager defaults to dark") {
    ThemeManager mgr(std::filesystem::temp_directory_path());
    REQUIRE(mgr.current().name == "dark");
}

TEST_CASE("ThemeManager set_theme switches") {
    ThemeManager mgr(std::filesystem::temp_directory_path());
    mgr.set_theme("light");
    REQUIRE(mgr.current().name == "light");
    mgr.set_theme("monokai");
    REQUIRE(mgr.current().name == "monokai");
}

TEST_CASE("ThemeManager set_theme ignores unknown") {
    ThemeManager mgr(std::filesystem::temp_directory_path());
    mgr.set_theme("nonexistent");
    REQUIRE(mgr.current().name == "dark");
}

TEST_CASE("ThemeManager available_themes") {
    ThemeManager mgr(std::filesystem::temp_directory_path());
    auto themes = mgr.available_themes();
    REQUIRE(themes.size() == 3);
}

TEST_CASE("ThemeManager color_for") {
    ThemeManager mgr(std::filesystem::temp_directory_path());
    REQUIRE(mgr.color_for("primary_text").r == 212);
    REQUIRE(mgr.color_for("background").r == 30);
    REQUIRE(mgr.color_for("nonexistent").r == 212);
}

TEST_CASE("Dark and light have different backgrounds") {
    REQUIRE(Theme::dark().colors.background.r < Theme::light().colors.background.r);
}
