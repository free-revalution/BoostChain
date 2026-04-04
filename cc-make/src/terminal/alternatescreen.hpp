#pragma once
#include <cstdio>

namespace ccmake {

class AlternateScreen {
public:
    AlternateScreen() {
        // Write escape sequence directly to stdout
        fputs("\x1b[?1049h", stdout);
        fflush(stdout);
    }
    ~AlternateScreen() {
        fputs("\x1b[?1049l", stdout);
        fflush(stdout);
    }
    // Non-copyable
    AlternateScreen(const AlternateScreen&) = delete;
    AlternateScreen& operator=(const AlternateScreen&) = delete;
};

}  // namespace ccmake
