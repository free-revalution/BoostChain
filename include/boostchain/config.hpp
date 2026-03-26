#pragma once

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
#define BOOSTCHAIN_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define BOOSTCHAIN_PLATFORM_MACOS
#elif defined(__linux__)
#define BOOSTCHAIN_PLATFORM_LINUX
#endif

// Compiler detection
#if defined(_MSC_VER)
#define BOOSTCHAIN_COMPILER_MSVC
#elif defined(__clang__)
#define BOOSTCHAIN_COMPILER_CLANG
#elif defined(__GNUC__)
#define BOOSTCHAIN_COMPILER_GCC
#endif

// Exception control
#ifdef BOOSTCHAIN_NO_EXCEPTIONS
#define BOOSTCHAIN_THROW(msg) std::abort()
#else
#define BOOSTCHAIN_THROW(msg) throw std::runtime_error(msg)
#endif
