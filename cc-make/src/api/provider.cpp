#include "api/provider.hpp"
#include "auth/auth.hpp"

namespace ccmake {

std::string normalize_model_string(const std::string& model) {
    // Future: strip common prefixes, resolve aliases.
    // For now, pass through as-is.
    return model;
}

std::string resolve_model(const ProviderConfig& config) {
    if (!config.model.empty()) {
        return config.model;
    }
    auto env_model = get_model_from_env();
    if (env_model.has_value()) {
        return env_model.value();
    }
    return "claude-sonnet-4-20250514";
}

std::unique_ptr<ClaudeClient> create_provider_client(const ProviderConfig& config) {
    ClaudeClientConfig client_config;
    client_config.model = resolve_model(config);
    client_config.timeout_ms = config.timeout_ms;
    client_config.max_retries = config.max_retries;
    client_config.api_key = config.api_key;

    switch (config.provider) {
        case APIProvider::Bedrock: {
            std::string bedrock_url = "https://bedrock-runtime." + config.region + ".amazonaws.com";
            client_config.base_url = bedrock_url;
            break;
        }
        case APIProvider::Vertex: {
            std::string vertex_url = "https://" + config.region + "-aiplatform.googleapis.com";
            client_config.base_url = vertex_url;
            break;
        }
        case APIProvider::FirstParty:
        default:
            client_config.base_url = config.base_url;
            break;
    }

    return std::make_unique<ClaudeClient>(std::move(client_config));
}

ProviderConfig build_provider_config(
    std::optional<std::string> api_key,
    std::optional<std::string> model,
    std::optional<std::string> base_url
) {
    ProviderConfig config;
    config.provider = get_api_provider();

    if (api_key.has_value()) {
        config.api_key = api_key.value();
    } else {
        auto key_info = get_api_key_with_source();
        if (key_info.has_value()) {
            config.api_key = key_info->key;
        }
    }

    if (model.has_value()) {
        config.model = model.value();
    } else {
        auto env_model = get_model_from_env();
        if (env_model.has_value()) {
            config.model = env_model.value();
        }
    }

    if (base_url.has_value()) {
        config.base_url = base_url.value();
    } else {
        config.base_url = get_api_base_url();
    }

    // Set provider-specific base URLs
    switch (config.provider) {
        case APIProvider::Bedrock: {
            const char* region_env = std::getenv("AWS_DEFAULT_REGION");
            config.region = region_env ? region_env : "us-east-1";
            config.base_url = "https://bedrock-runtime." + config.region + ".amazonaws.com";
            break;
        }
        case APIProvider::Vertex: {
            const char* region_env = std::getenv("CLOUD_ML_REGION");
            config.region = region_env ? region_env : "us-east5";
            const char* project_env = std::getenv("GOOGLE_CLOUD_PROJECT");
            config.project_id = project_env ? project_env : "";
            config.base_url = "https://" + config.region + "-aiplatform.googleapis.com";
            break;
        }
        case APIProvider::FirstParty:
        default:
            // Already set from env or parameter above
            break;
    }

    return config;
}

}  // namespace ccmake
