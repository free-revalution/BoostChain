#include <catch2/catch_test_macros.hpp>
#include "api/errors.hpp"

using namespace ccmake;

TEST_CASE("parse_api_error from 401") {
    auto err = parse_api_error(401, R"({"type":"authentication_error","error":{"type":"authentication_error","message":"invalid x-api-key"}})");
    REQUIRE(err.status_code == 401);
    REQUIRE(err.error_type == "authentication_error");
    REQUIRE(err.should_retry == false);
}

TEST_CASE("parse_api_error from 429") {
    auto err = parse_api_error(429, R"({"type":"error","error":{"type":"rate_limit_error","message":"Rate limit"}})");
    REQUIRE(err.should_retry == true);
    REQUIRE(err.error_type == "rate_limit_error");
}

TEST_CASE("parse_api_error from 529 overloaded") {
    auto err = parse_api_error(529, R"({"type":"error","error":{"type":"overloaded_error","message":"Overloaded"}})");
    REQUIRE(err.is_overloaded == true);
    REQUIRE(err.should_retry == true);
}

TEST_CASE("parse_api_error from 500") {
    auto err = parse_api_error(500, R"({"type":"error","error":{"type":"api_error","message":"Internal"}})");
    REQUIRE(err.should_retry == true);
}

TEST_CASE("parse_api_error from 400") {
    auto err = parse_api_error(400, R"({"type":"error","error":{"type":"invalid_request_error","message":"Bad"}})");
    REQUIRE(err.should_retry == false);
}

TEST_CASE("parse_api_error malformed JSON") {
    auto err = parse_api_error(500, "not json");
    REQUIRE(err.should_retry == true);  // 5xx defaults to retryable
}

TEST_CASE("make_connection_error") {
    auto err = make_connection_error("timed out");
    REQUIRE(err.status_code == 0);
    REQUIRE(err.should_retry == true);
    REQUIRE(err.error_type == "connection_error");
}

TEST_CASE("should_retry_error") {
    APIError err;
    err.status_code = 200;
    REQUIRE_FALSE(should_retry_error(err));
    err.status_code = 429;
    REQUIRE(should_retry_error(err));
    err.status_code = 529;
    err.is_overloaded = true;
    REQUIRE(should_retry_error(err));
    err.status_code = 408;
    REQUIRE(should_retry_error(err));
    err.status_code = 400;
    err.is_overloaded = false;
    err.should_retry = false;
    REQUIRE_FALSE(should_retry_error(err));
    err.status_code = 0;
    err.should_retry = true;
    REQUIRE(should_retry_error(err));
}

TEST_CASE("get_retry_delay_ms exponential backoff") {
    int d1 = get_retry_delay_ms(1, std::nullopt);
    REQUIRE(d1 >= 500);
    REQUIRE(d1 <= 625);
    int d5 = get_retry_delay_ms(5, std::nullopt);
    REQUIRE(d5 >= 8000);
    REQUIRE(d5 <= 10000);
    int d10 = get_retry_delay_ms(10, std::nullopt);
    REQUIRE(d10 >= 32000);
    REQUIRE(d10 <= 40000);
}

TEST_CASE("get_retry_delay_ms with retry-after header") {
    REQUIRE(get_retry_delay_ms(1, "10") == 10000);
    REQUIRE(get_retry_delay_ms(5, "3") == 3000);
}

TEST_CASE("user_facing_error_message auth") {
    APIError err;
    err.status_code = 401;
    err.error_type = "authentication_error";
    err.message = "invalid key";
    auto msg = user_facing_error_message(err);
    REQUIRE(msg.find("Authentication") != std::string::npos);
}

TEST_CASE("user_facing_error_message rate limit") {
    APIError err;
    err.status_code = 429;
    err.error_type = "rate_limit_error";
    err.message = "Rate limit";
    auto msg = user_facing_error_message(err);
    REQUIRE(msg.find("Rate") != std::string::npos);
}

TEST_CASE("user_facing_error_message connection") {
    APIError err;
    err.status_code = 0;
    err.error_type = "connection_error";
    err.message = "timed out";
    auto msg = user_facing_error_message(err);
    REQUIRE(msg.find("Connection") != std::string::npos);
}

TEST_CASE("user_facing_error_message overloaded") {
    APIError err;
    err.status_code = 529;
    err.is_overloaded = true;
    err.message = "Overloaded";
    auto msg = user_facing_error_message(err);
    REQUIRE(!msg.empty());
}

TEST_CASE("user_facing_error_message default") {
    APIError err;
    err.status_code = 500;
    err.error_type = "server_error";
    err.message = "Internal";
    auto msg = user_facing_error_message(err);
    REQUIRE(msg.find("500") != std::string::npos);
}
