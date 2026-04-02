#include <catch2/catch_test_macros.hpp>
#include <cstdlib>

#include "auth/auth.hpp"

using namespace ccmake;

TEST_CASE("get_api_key reads from env var") {
    // Ensure clean state
    ::unsetenv("ANTHROPIC_API_KEY");

    ::setenv("ANTHROPIC_API_KEY", "sk-ant-testkey123", 1);
    auto key = get_api_key();
    REQUIRE(key.has_value());
    REQUIRE(*key == "sk-ant-testkey123");

    ::unsetenv("ANTHROPIC_API_KEY");
}

TEST_CASE("get_api_key returns nullopt when not set") {
    ::unsetenv("ANTHROPIC_API_KEY");

    auto key = get_api_key();
    REQUIRE_FALSE(key.has_value());
}

TEST_CASE("get_api_provider defaults to FirstParty") {
    ::unsetenv("CLAUDE_CODE_USE_BEDROCK");
    ::unsetenv("CLAUDE_CODE_USE_VERTEX");
    ::unsetenv("CLAUDE_CODE_USE_FOUNDRY");

    REQUIRE(get_api_provider() == APIProvider::FirstParty);
}

TEST_CASE("get_api_provider reads Bedrock env") {
    ::unsetenv("CLAUDE_CODE_USE_BEDROCK");
    ::unsetenv("CLAUDE_CODE_USE_VERTEX");
    ::unsetenv("CLAUDE_CODE_USE_FOUNDRY");

    ::setenv("CLAUDE_CODE_USE_BEDROCK", "1", 1);
    REQUIRE(get_api_provider() == APIProvider::Bedrock);

    ::unsetenv("CLAUDE_CODE_USE_BEDROCK");
}

TEST_CASE("get_api_provider reads Vertex env") {
    ::unsetenv("CLAUDE_CODE_USE_BEDROCK");
    ::unsetenv("CLAUDE_CODE_USE_VERTEX");
    ::unsetenv("CLAUDE_CODE_USE_FOUNDRY");

    ::setenv("CLAUDE_CODE_USE_VERTEX", "1", 1);
    REQUIRE(get_api_provider() == APIProvider::Vertex);

    ::unsetenv("CLAUDE_CODE_USE_VERTEX");
}

TEST_CASE("get_api_provider reads Foundry env") {
    ::unsetenv("CLAUDE_CODE_USE_BEDROCK");
    ::unsetenv("CLAUDE_CODE_USE_VERTEX");
    ::unsetenv("CLAUDE_CODE_USE_FOUNDRY");

    ::setenv("CLAUDE_CODE_USE_FOUNDRY", "1", 1);
    REQUIRE(get_api_provider() == APIProvider::Foundry);

    ::unsetenv("CLAUDE_CODE_USE_FOUNDRY");
}

TEST_CASE("get_model_from_env reads ANTHROPIC_MODEL") {
    ::unsetenv("ANTHROPIC_MODEL");

    ::setenv("ANTHROPIC_MODEL", "claude-3-opus-20240229", 1);
    auto model = get_model_from_env();
    REQUIRE(model.has_value());
    REQUIRE(*model == "claude-3-opus-20240229");

    ::unsetenv("ANTHROPIC_MODEL");
}
