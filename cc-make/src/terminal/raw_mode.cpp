#include "raw_mode.hpp"

#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <cstring>

namespace ccmake {

struct TerminalRawMode::Impl {
    termios original_termios{};
    bool enabled = false;
    InputCallback input_callback;
};

TerminalRawMode::TerminalRawMode() : impl_(std::make_unique<Impl>()) {}

TerminalRawMode::~TerminalRawMode() {
    if (is_enabled()) {
        disable();
    }
}

bool TerminalRawMode::enable() {
    if (impl_->enabled) return true;
    if (!isatty(STDIN_FILENO)) return false;

    if (tcgetattr(STDIN_FILENO, &impl_->original_termios) != 0) {
        return false;
    }

    termios raw = impl_->original_termios;
    // Disable canonical mode, echo, signal processing, flow control, extension processing
    raw.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
    // Disable software flow control
    raw.c_iflag &= ~(IXON);
    // Disable output processing
    raw.c_oflag &= ~(OPOST);
    // Set minimum character count and timeout for blocking reads
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        return false;
    }

    impl_->enabled = true;
    return true;
}

bool TerminalRawMode::disable() {
    if (!impl_->enabled) return true;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &impl_->original_termios) != 0) {
        return false;
    }

    impl_->enabled = false;
    return true;
}

bool TerminalRawMode::is_enabled() const {
    return impl_->enabled;
}

int TerminalRawMode::get_width() {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        return 80; // fallback
    }
    return ws.ws_col;
}

int TerminalRawMode::get_height() {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        return 24; // fallback
    }
    return ws.ws_row;
}

bool TerminalRawMode::is_tty() {
    return isatty(STDIN_FILENO) != 0;
}

void TerminalRawMode::set_input_callback(InputCallback cb) {
    impl_->input_callback = std::move(cb);
}

void TerminalRawMode::poll_input() {
    struct pollfd pfd;
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;

    int result = ::poll(&pfd, 1, 0); // non-blocking, 0ms timeout
    if (result > 0 && (pfd.revents & POLLIN)) {
        std::string input = read_input();
        if (impl_->input_callback && !input.empty()) {
            impl_->input_callback(input);
        }
    }
}

std::string TerminalRawMode::read_input() {
    std::string result;
    char buf[256];
    ssize_t n;
    while ((n = ::read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        result.append(buf, static_cast<size_t>(n));
    }
    return result;
}

}  // namespace ccmake
