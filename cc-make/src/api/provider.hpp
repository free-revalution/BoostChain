#pragma once

#include "api/claude_client.hpp"
#include "core/types.hpp"
#include <memory>
#include <string>
#include <optional>
#include <vector>

namespace ccmake {

struct ProviderConfig {
    APIProvider provider = APIProvider::FirstParty;
    std::string base_url = "https://api.anthropic.com";
    std::string api_key;
    std::string model;
    std::string region;
    std::string project_id;
    int timeout_ms = 600000;
    int max_retries = 10;
    std::vector<std::string> betas;
};

std::string normalize_model_string(const std::string& model);
std::string resolve_model(const ProviderConfig& config);
std::unique_ptr<ClaudeClient> create_provider_client(const ProviderConfig& config);
ProviderConfig build_provider_config(
    std::optional<std::string> api_key = std::nullopt,
    std::optional<std::string> model = std::nullopt,
    std::optional<std::string> base_url = std::nullopt
);

}  // namespace ccmake
