#include "utils/process.hpp"
#include "utils/platform.hpp"

#include <array>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>

namespace ccmake {

namespace {

/// Set a file descriptor to non-blocking mode.
bool set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

/// Read all available data from a file descriptor without blocking.
/// Returns true if more data might be available, false on EOF or error.
bool read_available(int fd, std::string& buffer) {
    std::array<char, 4096> buf;
    ssize_t n;
    while ((n = ::read(fd, buf.data(), buf.size())) > 0) {
        buffer.append(buf.data(), static_cast<size_t>(n));
    }
    if (n == 0) return false;  // EOF
    if (n == -1 && errno == EAGAIN) return true;  // more data may come later
    return false;  // error
}

}  // anonymous namespace

CommandResult run_command(
    const std::string& command,
    const std::optional<std::string>& working_dir,
    const std::optional<std::chrono::milliseconds>& timeout,
    const std::vector<std::pair<std::string, std::string>>& env
) {
    CommandResult result;

    // Create pipes for stdout and stderr.
    int stdout_pipe[2];
    int stderr_pipe[2];
    if (pipe(stdout_pipe) == -1) {
        result.stderr_output = "pipe() failed for stdout: " + std::string(std::strerror(errno));
        return result;
    }
    if (pipe(stderr_pipe) == -1) {
        result.stderr_output = "pipe() failed for stderr: " + std::string(std::strerror(errno));
        ::close(stdout_pipe[0]);
        ::close(stdout_pipe[1]);
        return result;
    }

    pid_t pid = fork();
    if (pid == -1) {
        result.stderr_output = "fork() failed: " + std::string(std::strerror(errno));
        ::close(stdout_pipe[0]);
        ::close(stdout_pipe[1]);
        ::close(stderr_pipe[0]);
        ::close(stderr_pipe[1]);
        return result;
    }

    if (pid == 0) {
        // Child process.
        ::close(stdout_pipe[0]);
        ::close(stderr_pipe[0]);

        ::dup2(stdout_pipe[1], STDOUT_FILENO);
        ::dup2(stderr_pipe[1], STDERR_FILENO);
        ::close(stdout_pipe[1]);
        ::close(stderr_pipe[1]);

        // Change working directory if specified.
        if (working_dir.has_value()) {
            if (::chdir(working_dir->c_str()) != 0) {
                _exit(127);
            }
        }

        // Set additional environment variables.
        for (const auto& [key, value] : env) {
            ::setenv(key.c_str(), value.c_str(), 1);
        }

        ::execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
        _exit(127);  // exec failed
    }

    // Parent process.
    ::close(stdout_pipe[1]);
    ::close(stderr_pipe[1]);

    // Set read ends to non-blocking.
    set_nonblocking(stdout_pipe[0]);
    set_nonblocking(stderr_pipe[0]);

    // Compute deadline for timeout.
    auto deadline = std::chrono::steady_clock::now() + timeout.value_or(std::chrono::milliseconds(30000));

    bool stdout_eof = false;
    bool stderr_eof = false;
    bool killed = false;

    while (!stdout_eof || !stderr_eof) {
        auto now = std::chrono::steady_clock::now();
        auto remaining = deadline - now;
        if (remaining <= std::chrono::milliseconds(0)) {
            // Timeout expired.
            ::kill(pid, SIGKILL);
            killed = true;
            result.timed_out = true;
            break;
        }

        pollfd fds[2];
        int nfds = 0;

        if (!stdout_eof) {
            fds[nfds].fd = stdout_pipe[0];
            fds[nfds].events = POLLIN;
            fds[nfds].revents = 0;
            ++nfds;
        }
        if (!stderr_eof) {
            fds[nfds].fd = stderr_pipe[0];
            fds[nfds].events = POLLIN;
            fds[nfds].revents = 0;
            ++nfds;
        }

        int ms = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(remaining).count()
        );

        int poll_result = ::poll(fds, static_cast<nfds_t>(nfds), ms);

        if (poll_result == -1) {
            if (errno == EINTR) continue;
            break;
        }

        if (poll_result == 0) {
            // Timeout on poll - check deadline on next iteration.
            continue;
        }

        // Check which fds have data.
        int fd_idx = 0;
        if (!stdout_eof) {
            if (fds[fd_idx].revents & (POLLIN | POLLHUP)) {
                if (!read_available(stdout_pipe[0], result.stdout_output)) {
                    stdout_eof = true;
                }
            }
            if (fds[fd_idx].revents & (POLLERR | POLLNVAL)) {
                stdout_eof = true;
            }
            ++fd_idx;
        }
        if (!stderr_eof) {
            if (fds[fd_idx].revents & (POLLIN | POLLHUP)) {
                if (!read_available(stderr_pipe[0], result.stderr_output)) {
                    stderr_eof = true;
                }
            }
            if (fds[fd_idx].revents & (POLLERR | POLLNVAL)) {
                stderr_eof = true;
            }
        }
    }

    ::close(stdout_pipe[0]);
    ::close(stderr_pipe[0]);

    // Wait for the child to finish.
    int status = 0;
    pid_t waited;
    do {
        waited = ::waitpid(pid, &status, 0);
    } while (waited == -1 && errno == EINTR);

    if (waited != -1 && WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else if (waited != -1 && WIFSIGNALED(status)) {
        result.exit_code = 128 + WTERMSIG(status);
    }

    return result;
}

// ---------------------------------------------------------------------------
// AsyncProcess - POSIX implementation
// ---------------------------------------------------------------------------

class UnixAsyncProcess : public AsyncProcess {
public:
    UnixAsyncProcess(int p, int stdin_fd, int stdout_fd, int stderr_fd)
        : pid_(p), stdin_fd_(stdin_fd), stdout_fd_(stdout_fd), stderr_fd_(stderr_fd),
          stdout_eof_(false), stderr_eof_(false) {}

    ~UnixAsyncProcess() override {
        close_fd(stdin_fd_);
        close_fd(stdout_fd_);
        close_fd(stderr_fd_);
        // If the child is still running, try to reap it.
        if (pid_ > 0) {
            int status;
            ::waitpid(pid_, &status, WNOHANG);
        }
    }

    std::optional<std::string> read_stdout() override {
        return read_from(stdout_fd_, stdout_eof_);
    }

    std::optional<std::string> read_stderr() override {
        return read_from(stderr_fd_, stderr_eof_);
    }

    bool write_stdin(const std::string& data) override {
        if (stdin_fd_ == -1) return false;
        ssize_t n = ::write(stdin_fd_, data.data(), data.size());
        if (n == -1) return false;
        return static_cast<size_t>(n) == data.size();
    }

    int wait() override {
        if (pid_ <= 0) return -1;
        int status = 0;
        pid_t waited;
        do {
            waited = ::waitpid(pid_, &status, 0);
        } while (waited == -1 && errno == EINTR);

        if (waited == -1) return -1;
        if (WIFEXITED(status)) return WEXITSTATUS(status);
        if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
        return -1;
    }

    bool kill() override {
        if (pid_ <= 0) return false;
        return ::kill(pid_, SIGKILL) == 0;
    }

    bool is_running() override {
        if (pid_ <= 0) return false;
        int status = 0;
        pid_t result = ::waitpid(pid_, &status, WNOHANG);
        if (result == 0) return true;   // still running
        if (result == -1) return false;  // error
        return false;  // reaped
    }

    int pid() const override { return pid_; }

private:
    std::optional<std::string> read_from(int fd, bool& eof_flag) {
        if (fd == -1) return std::nullopt;
        if (eof_flag) return std::nullopt;

        std::array<char, 4096> buf;
        ssize_t n = ::read(fd, buf.data(), buf.size());
        if (n > 0) {
            return std::string(buf.data(), static_cast<size_t>(n));
        }
        if (n == 0) {
            eof_flag = true;
            return std::nullopt;
        }
        // n == -1
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return std::string{};  // empty string = no data right now
        }
        eof_flag = true;
        return std::nullopt;
    }

    static void close_fd(int& fd) {
        if (fd != -1) {
            ::close(fd);
            fd = -1;
        }
    }

    int pid_;
    int stdin_fd_;
    int stdout_fd_;
    int stderr_fd_;
    bool stdout_eof_;
    bool stderr_eof_;
};

std::unique_ptr<AsyncProcess> run_command_async(
    const std::string& command,
    const std::optional<std::string>& working_dir,
    const std::vector<std::pair<std::string, std::string>>& env
) {
    int stdin_pipe[2];
    int stdout_pipe[2];
    int stderr_pipe[2];

    if (pipe(stdin_pipe) == -1) {
        throw std::runtime_error("pipe() failed for stdin: " + std::string(std::strerror(errno)));
    }
    if (pipe(stdout_pipe) == -1) {
        throw std::runtime_error("pipe() failed for stdout: " + std::string(std::strerror(errno)));
    }
    if (pipe(stderr_pipe) == -1) {
        throw std::runtime_error("pipe() failed for stderr: " + std::string(std::strerror(errno)));
    }

    pid_t pid = fork();
    if (pid == -1) {
        ::close(stdin_pipe[0]);
        ::close(stdin_pipe[1]);
        ::close(stdout_pipe[0]);
        ::close(stdout_pipe[1]);
        ::close(stderr_pipe[0]);
        ::close(stderr_pipe[1]);
        throw std::runtime_error("fork() failed: " + std::string(std::strerror(errno)));
    }

    if (pid == 0) {
        // Child process.
        ::close(stdin_pipe[1]);   // close parent's write end
        ::close(stdout_pipe[0]);  // close parent's read end
        ::close(stderr_pipe[0]);  // close parent's read end

        ::dup2(stdin_pipe[0], STDIN_FILENO);
        ::dup2(stdout_pipe[1], STDOUT_FILENO);
        ::dup2(stderr_pipe[1], STDERR_FILENO);

        ::close(stdin_pipe[0]);
        ::close(stdout_pipe[1]);
        ::close(stderr_pipe[1]);

        if (working_dir.has_value()) {
            if (::chdir(working_dir->c_str()) != 0) {
                _exit(127);
            }
        }

        for (const auto& [key, value] : env) {
            ::setenv(key.c_str(), value.c_str(), 1);
        }

        ::execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
        _exit(127);
    }

    // Parent process.
    ::close(stdin_pipe[0]);   // close child's read end
    ::close(stdout_pipe[1]);  // close child's write end
    ::close(stderr_pipe[1]);  // close child's write end

    set_nonblocking(stdout_pipe[0]);
    set_nonblocking(stderr_pipe[0]);

    return std::make_unique<UnixAsyncProcess>(
        pid,
        stdin_pipe[1],   // parent writes to child's stdin
        stdout_pipe[0],  // parent reads from child's stdout
        stderr_pipe[0]   // parent reads from child's stderr
    );
}

}  // namespace ccmake
