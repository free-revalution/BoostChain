#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "utils/process.hpp"

#include <chrono>
#include <string>
#include <thread>

using namespace ccmake;
using namespace std::chrono_literals;

TEST_CASE("run_command captures stdout") {
    auto result = run_command("echo hello");
    REQUIRE(result.exit_code == 0);
    REQUIRE(result.stdout_output == "hello\n");
    REQUIRE(result.stderr_output.empty());
    REQUIRE_FALSE(result.timed_out);
}

TEST_CASE("run_command captures stderr") {
    auto result = run_command("echo error >&2");
    REQUIRE(result.exit_code == 0);
    REQUIRE_THAT(result.stderr_output, Catch::Matchers::ContainsSubstring("error"));
}

TEST_CASE("run_command returns non-zero exit code") {
    auto result = run_command("exit 42");
    REQUIRE(result.exit_code == 42);
}

TEST_CASE("run_command respects working directory") {
    auto result = run_command("pwd", "/");
    REQUIRE(result.exit_code == 0);
    REQUIRE(result.stdout_output == "/\n");
}

TEST_CASE("run_command respects timeout") {
    auto result = run_command("sleep 100", std::nullopt, 100ms);
    REQUIRE(result.timed_out);
    // The exit code should indicate the process was killed.
    REQUIRE(result.exit_code == 128 + 9);  // SIGKILL = 9
}

TEST_CASE("run_command_async streams output") {
    auto proc = run_command_async("echo async-output");
    REQUIRE(proc->is_running());
    REQUIRE(proc->pid() > 0);

    // Give the child a moment to produce output.
    std::this_thread::sleep_for(50ms);

    // Read available output.
    std::string combined;
    while (true) {
        auto chunk = proc->read_stdout();
        if (!chunk.has_value()) break;
        combined += *chunk;
        if (chunk->empty()) {
            // No data available yet; wait a bit.
            std::this_thread::sleep_for(10ms);
        }
    }

    int exit_code = proc->wait();
    REQUIRE(exit_code == 0);
    REQUIRE_THAT(combined, Catch::Matchers::ContainsSubstring("async-output"));
    REQUIRE_FALSE(proc->is_running());
}
