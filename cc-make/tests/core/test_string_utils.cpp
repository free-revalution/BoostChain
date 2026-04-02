#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "core/string_utils.hpp"

using namespace ccmake;

TEST_CASE("trim removes whitespace including tabs and newlines") {
    REQUIRE(trim("  hello  ") == "hello");
    REQUIRE(trim("\thello\t") == "hello");
    REQUIRE(trim("\nhello\n") == "hello");
    REQUIRE(trim("  \t hello world \n  ") == "hello world");
    REQUIRE(trim("") == "");
    REQUIRE(trim("   ") == "");
}

TEST_CASE("split on delimiter") {
    auto parts = split("a,b,c", ',');
    REQUIRE(parts.size() == 3);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1] == "b");
    REQUIRE(parts[2] == "c");
}

TEST_CASE("split with empty segments") {
    auto parts = split("a,,c", ',');
    REQUIRE(parts.size() == 3);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1] == "");
    REQUIRE(parts[2] == "c");
}

TEST_CASE("starts_with and ends_with") {
    REQUIRE(starts_with("hello world", "hello"));
    REQUIRE_FALSE(starts_with("hello world", "world"));
    REQUIRE_FALSE(starts_with("hi", "hello"));

    REQUIRE(ends_with("hello world", "world"));
    REQUIRE_FALSE(ends_with("hello world", "hello"));
    REQUIRE_FALSE(ends_with("hi", "hello"));
}

TEST_CASE("contains") {
    REQUIRE(contains("hello world", "hello"));
    REQUIRE(contains("hello world", "world"));
    REQUIRE(contains("hello world", "o w"));
    REQUIRE_FALSE(contains("hello world", "xyz"));
    REQUIRE_FALSE(contains("", "a"));
}

TEST_CASE("to_lower") {
    REQUIRE(to_lower("HELLO") == "hello");
    REQUIRE(to_lower("Hello World") == "hello world");
    REQUIRE(to_lower("") == "");
}

TEST_CASE("join") {
    REQUIRE(join({"a", "b", "c"}, ", ") == "a, b, c");
    REQUIRE(join({"single"}, ", ") == "single");
    REQUIRE(join({"", "x", ""}, "-") == "-x-");
}

TEST_CASE("join empty returns empty") {
    REQUIRE(join({}, ", ") == "");
}
