#pragma once

#include "core/types.hpp"
#include <string>
#include <optional>

namespace ccmake {

std::optional<std::string> get_api_key();

struct ApiKeyInfo {
    std::string key;
    std::string source;
};

std::optional<ApiKeyInfo> get_api_key_with_source();

APIProvider get_api_provider();

std::optional<std::string> get_model_from_env();

bool is_valid_api_key_format(const std::string& key);

std::string get_api_base_url();

std::string get_current_working_directory();

}  // namespace ccmake
