#pragma once

#include <boostchain/version.hpp>
#include <boostchain/config.hpp>
#include <boostchain/llm_provider.hpp>

namespace boostchain {
    inline constexpr int version_major() { return BOOSTCHAIN_VERSION_MAJOR; }
    inline constexpr int version_minor() { return BOOSTCHAIN_VERSION_MINOR; }
    inline constexpr int version_patch() { return BOOSTCHAIN_VERSION_PATCH; }
    inline constexpr const char* version() { return BOOSTCHAIN_VERSION; }
}
