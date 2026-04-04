#include <catch2/catch_test_macros.hpp>

#include "core/types.hpp"
#include "core/result.hpp"
#include "config/config.hpp"
#include "auth/auth.hpp"
#include "utils/process.hpp"
#include "utils/filesystem.hpp"

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

using namespace ccmake;

// ---------------------------------------------------------------------------
// Test 1: Config loads and auth detects provider
// ---------------------------------------------------------------------------

TEST_CASE("End-to-end: Config loads and auth detects provider") {
    // Loading from a nonexistent path returns nullopt (no file = no config).
    auto cfg = Config::load_from_file("/nonexistent");
    REQUIRE_FALSE(cfg.has_value());

    // A default-constructed Config has sensible defaults.
    Config defaults;
    REQUIRE(defaults.permission_mode == PermissionMode::Default);
    REQUIRE(defaults.theme == "dark");
    REQUIRE(defaults.auto_compact == true);

    // In a default environment (no provider-specific env vars set), auth should
    // detect FirstParty provider.
    auto provider = get_api_provider();
    REQUIRE(provider == APIProvider::FirstParty);
}

// ---------------------------------------------------------------------------
// Test 2: Process + Filesystem workflow
// ---------------------------------------------------------------------------

TEST_CASE("End-to-end: Process + Filesystem workflow") {
    auto dir = fs::temp_directory_path() / "ccmake_integration";
    fs::remove_all(dir);  // Clean up any previous run.
    fs::create_directories(dir);

    // Write a file.
    auto path = dir / "test.txt";
    REQUIRE(write_file(path, "hello"));

    // Read it back.
    auto content = read_file(path);
    REQUIRE(content.has_value());
    REQUIRE(content.value() == "hello");

    // Use process to cat the file.
    auto result = run_command("cat " + path.string());
    REQUIRE(result.exit_code == 0);
    REQUIRE(result.stdout_output == "hello");

    // Clean up.
    fs::remove_all(dir);
}

// ---------------------------------------------------------------------------
// Test 3: Core types work together
// ---------------------------------------------------------------------------

TEST_CASE("End-to-end: Core types work together") {
    // Create a user message.
    auto user_msg = Message::user("id-1", "Hello");
    REQUIRE(user_msg.role == MessageRole::User);

    // Create an assistant message with a text block and stop reason.
    auto asst_msg = Message::assistant(
        "id-2",
        {ContentBlock{TextBlock{"Hi there!"}}},
        StopReason::EndTurn
    );
    REQUIRE(asst_msg.role == MessageRole::Assistant);

    // Chain Result operations: ok(42) -> map to string -> and_then to int.
    auto chained = Result<int, std::string>::ok(42)
        .map([](int v) { return std::to_string(v); })
        .and_then([](const std::string& s) -> Result<int, std::string> {
            return Result<int, std::string>::ok(static_cast<int>(s.size()));
        });
    REQUIRE(chained.has_value());
    REQUIRE(chained.value() == 2);
}
