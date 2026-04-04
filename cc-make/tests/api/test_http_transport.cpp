#include <catch2/catch_test_macros.hpp>
#include "api/http_transport.hpp"

using namespace ccmake;

TEST_CASE("HTTPTransportConfig default values") {
    HTTPTransportConfig config;
    REQUIRE(config.base_url == "https://api.anthropic.com");
    REQUIRE(config.api_key.empty());
    REQUIRE(config.timeout_ms == 600000);
    REQUIRE(config.max_connect_timeout_ms == 30000);
}

TEST_CASE("HTTPTransportConfig build_headers includes standard headers") {
    HTTPTransportConfig config;
    config.api_key = "sk-test";
    auto headers = config.build_headers();
    bool has_content_type = false, has_auth = false, has_version = false;
    for (const auto& [k, v] : headers) {
        if (k == "Content-Type") has_content_type = true;
        if (k == "x-api-key" && v == "sk-test") has_auth = true;
        if (k == "anthropic-version") has_version = true;
    }
    REQUIRE(has_content_type);
    REQUIRE(has_auth);
    REQUIRE(has_version);
}

TEST_CASE("HTTPTransportConfig build_headers with betas") {
    HTTPTransportConfig config;
    config.betas = {"beta-1", "beta-2"};
    auto headers = config.build_headers();
    bool found = false;
    for (const auto& [k, v] : headers) {
        if (k == "anthropic-beta" && v == "beta-1,beta-2") found = true;
    }
    REQUIRE(found);
}

TEST_CASE("HTTPTransportConfig build_headers without api_key") {
    HTTPTransportConfig config;
    auto headers = config.build_headers();
    for (const auto& [k, v] : headers) {
        REQUIRE(k != "x-api-key");
    }
}

TEST_CASE("HTTPTransportConfig extra_headers") {
    HTTPTransportConfig config;
    config.extra_headers = {{"X-Custom", "value"}};
    auto headers = config.build_headers();
    bool found = false;
    for (const auto& [k, v] : headers) {
        if (k == "X-Custom" && v == "value") found = true;
    }
    REQUIRE(found);
}

TEST_CASE("HTTPTransportResponse default values") {
    HTTPTransportResponse resp;
    REQUIRE(resp.status_code == 0);
    REQUIRE(resp.body.empty());
    REQUIRE(resp.headers.empty());
}

TEST_CASE("HTTPTransportResponse stores headers") {
    HTTPTransportResponse resp;
    resp.status_code = 200;
    resp.headers.push_back({"content-type", "text/event-stream"});
    resp.headers.push_back({"retry-after", "5"});
    REQUIRE(resp.status_code == 200);
    REQUIRE(resp.headers.size() == 2);
    REQUIRE(resp.headers[1].second == "5");
}

TEST_CASE("HTTPTransport construction with config") {
    HTTPTransportConfig config;
    config.base_url = "https://api.example.com";
    config.api_key = "sk-test";
    HTTPTransport transport(config);
    REQUIRE(transport.config().base_url == "https://api.example.com");
    REQUIRE(transport.config().api_key == "sk-test");
}

TEST_CASE("HTTPTransportConfig build_headers includes Accept header") {
    HTTPTransportConfig config;
    auto headers = config.build_headers();
    bool has_accept = false;
    for (const auto& [k, v] : headers) {
        if (k == "Accept" && v == "text/event-stream") has_accept = true;
    }
    REQUIRE(has_accept);
}

TEST_CASE("HTTPTransportConfig build_headers includes User-Agent") {
    HTTPTransportConfig config;
    auto headers = config.build_headers();
    bool has_ua = false;
    for (const auto& [k, v] : headers) {
        if (k == "User-Agent" && v == "cc-make/0.1.0") has_ua = true;
    }
    REQUIRE(has_ua);
}

TEST_CASE("HTTPTransportConfig build_headers empty betas produces no beta header") {
    HTTPTransportConfig config;
    auto headers = config.build_headers();
    for (const auto& [k, v] : headers) {
        REQUIRE(k != "anthropic-beta");
    }
}
