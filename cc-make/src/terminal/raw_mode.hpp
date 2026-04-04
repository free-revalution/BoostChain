#pragma once
#include <functional>
#include <memory>
#include <string>

namespace ccmake {

class TerminalRawMode {
public:
    TerminalRawMode();
    ~TerminalRawMode();

    // Enable/disable raw mode
    bool enable();
    bool disable();
    bool is_enabled() const;

    // Get terminal size
    static int get_width();
    static int get_height();

    // Check if stdin is a TTY
    static bool is_tty();

    // Set callback for input
    using InputCallback = std::function<void(const std::string&)>;
    void set_input_callback(InputCallback cb);

    // Poll for input (non-blocking)
    void poll_input();

    // Read available input (blocking)
    std::string read_input();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace ccmake
