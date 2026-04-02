#pragma once

#include <string>

// Platform detection macros
#if defined(__APPLE__) && defined(__MACH__)
    #define CCMAKE_PLATFORM_MACOS 1
#elif defined(__linux__)
    #define CCMAKE_PLATFORM_LINUX 1
#elif defined(_WIN32) || defined(_WIN64)
    #define CCMAKE_PLATFORM_WINDOWS 1
#endif

namespace ccmake {

/// Enum representing the detected platform.
enum class Platform {
    MacOS,
    Linux,
    Windows,
    Unknown
};

/// Detect the current platform at runtime.
inline Platform get_platform() {
#if defined(CCMAKE_PLATFORM_MACOS)
    return Platform::MacOS;
#elif defined(CCMAKE_PLATFORM_LINUX)
    return Platform::Linux;
#elif defined(CCMAKE_PLATFORM_WINDOWS)
    return Platform::Windows;
#else
    return Platform::Unknown;
#endif
}

/// Returns true if the current platform is Unix-like (macOS or Linux).
inline bool is_unix() {
#if defined(CCMAKE_PLATFORM_MACOS) || defined(CCMAKE_PLATFORM_LINUX)
    return true;
#else
    return false;
#endif
}

/// Returns a human-readable name for the current platform.
inline std::string platform_name() {
    switch (get_platform()) {
        case Platform::MacOS:  return "macOS";
        case Platform::Linux:  return "Linux";
        case Platform::Windows: return "Windows";
        case Platform::Unknown: return "Unknown";
    }
    return "Unknown";
}

}  // namespace ccmake
