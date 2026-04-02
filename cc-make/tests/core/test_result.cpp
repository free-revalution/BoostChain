#include <catch2/catch_test_macros.hpp>
#include <string>

#include "core/result.hpp"

using namespace ccmake;

TEST_CASE("Result ok value retrieval") {
    auto r = Result<int, std::string>::ok(42);
    REQUIRE(r.has_value());
    REQUIRE(static_cast<bool>(r));
    REQUIRE(r.value() == 42);
}

TEST_CASE("Result err value retrieval") {
    auto r = Result<int, std::string>::err("something failed");
    REQUIRE_FALSE(r.has_value());
    REQUIRE_FALSE(static_cast<bool>(r));
    REQUIRE(r.error() == "something failed");
}

TEST_CASE("Result map transforms ok") {
    auto r = Result<int, std::string>::ok(10);
    auto mapped = r.map([](int v) { return v * 2; });
    REQUIRE(mapped.has_value());
    REQUIRE(mapped.value() == 20);
}

TEST_CASE("Result map propagates err") {
    auto r = Result<int, std::string>::err("oops");
    auto mapped = r.map([](int v) { return v * 2; });
    REQUIRE_FALSE(mapped.has_value());
    REQUIRE(mapped.error() == "oops");
}

TEST_CASE("Result and_then chains") {
    auto r = Result<int, std::string>::ok(5);
    auto chained = r.and_then([](int v) -> Result<std::string, std::string> {
        return Result<std::string, std::string>::ok("value: " + std::to_string(v));
    });
    REQUIRE(chained.has_value());
    REQUIRE(chained.value() == "value: 5");
}

TEST_CASE("Result value_or provides default") {
    auto ok_r = Result<int, std::string>::ok(10);
    REQUIRE(ok_r.value_or(99) == 10);

    auto err_r = Result<int, std::string>::err("bad");
    REQUIRE(err_r.value_or(99) == 99);
}

TEST_CASE("Result ok has no error - throws on error access") {
    auto r = Result<int, std::string>::ok(42);
    REQUIRE_THROWS_AS(r.error(), std::bad_optional_access);
}
