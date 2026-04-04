#include <catch2/catch_test_macros.hpp>
#include "api/provider.hpp"
#include "auth/auth.hpp"
#include "core/types.hpp"

using namespace ccmake;

TEST_CASE("ProviderConfig default is first party") {
    ProviderConfig config;
    REQUIRE(config.provider == APIProvider::FirstParty);
    REQUIRE(config.base_url == "https://api.anthropic.com");
}

TEST_CASE("ProviderConfig Bedrock") {
    ProviderConfig config;
    config.provider = APIProvider::Bedrock;
    config.region = "us-east-1";
    REQUIRE(config.provider == APIProvider::Bedrock);
}

TEST_CASE("ProviderConfig Vertex") {
    ProviderConfig config;
    config.provider = APIProvider::Vertex;
    config.region = "us-east5";
    REQUIRE(config.provider == APIProvider::Vertex);
}

TEST_CASE("normalize_model_string passes through") {
    REQUIRE(normalize_model_string("claude-sonnet-4-20250514") == "claude-sonnet-4-20250514");
    REQUIRE(normalize_model_string("claude-opus-4-20250514") == "claude-opus-4-20250514");
}

TEST_CASE("resolve_model uses config model") {
    ProviderConfig config;
    config.model = "claude-sonnet-4-20250514";
    REQUIRE(resolve_model(config) == "claude-sonnet-4-20250514");
}

TEST_CASE("resolve_model falls back to default") {
    ProviderConfig config;
    // model is empty, env may or may not be set
    auto m = resolve_model(config);
    REQUIRE(!m.empty());
}

TEST_CASE("create_provider_client first party") {
    ProviderConfig config;
    config.api_key = "sk-test-key";
    config.model = "claude-sonnet-4-20250514";
    auto client = create_provider_client(config);
    REQUIRE(client != nullptr);
    REQUIRE(client->transport().config().api_key == "sk-test-key");
    REQUIRE(client->transport().config().base_url == "https://api.anthropic.com");
}

TEST_CASE("create_provider_client with custom base URL") {
    ProviderConfig config;
    config.api_key = "sk-test";
    config.model = "claude-sonnet-4-20250514";
    config.base_url = "https://custom.api.example.com";
    auto client = create_provider_client(config);
    REQUIRE(client->transport().config().base_url == "https://custom.api.example.com");
}

TEST_CASE("build_provider_config from env") {
    auto config = build_provider_config();
    REQUIRE(config.provider == get_api_provider());
    REQUIRE(!config.base_url.empty());
}
