#pragma once

#include "core/result.hpp"

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ccmake {

/// The result of executing a command.
struct CommandResult {
    int exit_code = -1;
    std::string stdout_output;
    std::string stderr_output;
    bool timed_out = false;
};

/// Run a command synchronously and capture its output.
///
/// @param command     The shell command to execute (passed to /bin/sh -c).
/// @param working_dir Optional working directory for the child process.
/// @param timeout     Optional timeout; if exceeded the process is killed.
/// @param env         Optional additional environment variables.
/// @return CommandResult with exit code, stdout, stderr, and timeout status.
CommandResult run_command(
    const std::string& command,
    const std::optional<std::string>& working_dir = std::nullopt,
    const std::optional<std::chrono::milliseconds>& timeout = std::nullopt,
    const std::vector<std::pair<std::string, std::string>>& env = {}
);

/// Interface for an asynchronously running child process.
class AsyncProcess {
public:
    virtual ~AsyncProcess() = default;

    /// Try to read available stdout data. Returns nullopt on EOF, empty string if nothing available yet.
    virtual std::optional<std::string> read_stdout() = 0;

    /// Try to read available stderr data. Returns nullopt on EOF, empty string if nothing available yet.
    virtual std::optional<std::string> read_stderr() = 0;

    /// Write data to the child's stdin. Returns true on success.
    virtual bool write_stdin(const std::string& data) = 0;

    /// Block until the child process exits and return its exit code.
    virtual int wait() = 0;

    /// Kill the child process. Returns true on success.
    virtual bool kill() = 0;

    /// Check if the child process is still running.
    virtual bool is_running() = 0;

    /// Return the child's PID.
    virtual int pid() const = 0;
};

/// Launch a command asynchronously and return a handle to interact with it.
///
/// @param command     The shell command to execute (passed to /bin/sh -c).
/// @param working_dir Optional working directory for the child process.
/// @param env         Optional additional environment variables.
/// @return Unique pointer to an AsyncProcess handle.
std::unique_ptr<AsyncProcess> run_command_async(
    const std::string& command,
    const std::optional<std::string>& working_dir = std::nullopt,
    const std::vector<std::pair<std::string, std::string>>& env = {}
);

}  // namespace ccmake
