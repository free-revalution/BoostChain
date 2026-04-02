#include <catch2/catch_test_macros.hpp>
#include <string>

#include "core/json_utils.hpp"

using namespace ccmake;

TEST_CASE("json_get_or returns value if present") {
    nlohmann::json j = {{"name", "Alice"}, {"age", 30}};
    REQUIRE(json_get_or<std::string>(j, "name", "unknown") == "Alice");
    REQUIRE(json_get_or<int>(j, "age", 0) == 30);
}

TEST_CASE("json_get_or returns default if missing") {
    nlohmann::json j = {{"name", "Alice"}};
    REQUIRE(json_get_or<std::string>(j, "missing", "default") == "default");
    REQUIRE(json_get_or<int>(j, "age", -1) == -1);
}

TEST_CASE("json_safe_parse ok for valid JSON") {
    auto result = json_safe_parse("{\"key\": \"value\"}");
    REQUIRE(result.has_value());
    REQUIRE(result.value()["key"] == "value");
}

TEST_CASE("json_safe_parse err for invalid JSON") {
    auto result = json_safe_parse("not valid json");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().size() > 0);
}

TEST_CASE("json_get_string returns string or nullopt") {
    nlohmann::json j = {{"text", "hello"}, {"num", 42}};
    auto val = json_get_string(j, "text");
    REQUIRE(val.has_value());
    REQUIRE(val.value() == "hello");

    auto missing = json_get_string(j, "missing");
    REQUIRE_FALSE(missing.has_value());

    auto wrong_type = json_get_string(j, "num");
    REQUIRE_FALSE(wrong_type.has_value());
}
