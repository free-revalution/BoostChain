#include "auth.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>

namespace ccmake {

std::optional<std::string> get_api_key() {
    const char* env = std::getenv("ANTHROPIC_API_KEY");
    if (env == nullptr || env[0] == '\0') {
        return std::nullopt;
    }
    return std::string(env);
}

std::optional<ApiKeyInfo> get_api_key_with_source() {
    const char* env = std::getenv("ANTHROPIC_API_KEY");
    if (env == nullptr || env[0] == '\0') {
        return std::nullopt;
    }
    return ApiKeyInfo{std::string(env), "ANTHROPIC_API_KEY"};
}

APIProvider get_api_provider() {
    if (const char* env = std::getenv("CLAUDE_CODE_USE_BEDROCK")) {
        if (std::string(env) == "1") {
            return APIProvider::Bedrock;
        }
    }
    if (const char* env = std::getenv("CLAUDE_CODE_USE_VERTEX")) {
        if (std::string(env) == "1") {
            return APIProvider::Vertex;
        }
    }
    if (const char* env = std::getenv("CLAUDE_CODE_USE_FOUNDRY")) {
        if (std::string(env) == "1") {
            return APIProvider::Foundry;
        }
    }
    return APIProvider::FirstParty;
}

std::optional<std::string> get_model_from_env() {
    const char* env = std::getenv("ANTHROPIC_MODEL");
    if (env == nullptr || env[0] == '\0') {
        return std::nullopt;
    }
    return std::string(env);
}

bool is_valid_api_key_format(const std::string& key) {
    if (key.size() <= 10) {
        return false;
    }
    return key.rfind("sk-ant-", 0) == 0;
}

std::string get_api_base_url() {
    const char* env = std::getenv("ANTHROPIC_BASE_URL");
    if (env != nullptr && env[0] != '\0') {
        return std::string(env);
    }
    return "https://api.anthropic.com";
}

std::string get_current_working_directory() {
    return std::filesystem::current_path().string();
}

}  // namespace ccmake
