#pragma once

#include "api/types.hpp"
#include "core/types.hpp"
#include <string>
#include <vector>
#include <optional>

namespace ccmake {

APIMessageParam message_to_api_param(const Message& msg);

APIRequest build_api_request(
    const std::string& model,
    const std::vector<Message>& messages,
    const std::string& system_prompt,
    const std::vector<APIToolDefinition>& tools,
    int64_t max_tokens = 8192,
    const std::optional<APIThinkingConfig>& thinking = std::nullopt
);

}  // namespace ccmake
