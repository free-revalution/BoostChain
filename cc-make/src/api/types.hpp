#pragma once

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace ccmake {

// ============================================================
// API Request Types
// ============================================================

struct APIContentBlockText {
    std::string text;
    std::optional<std::string> cache_control;
};

struct APIContentBlockImage {
    std::string media_type;
    std::string data;
    std::optional<std::string> cache_control;
};

struct APIContentBlockToolResult {
    std::string tool_use_id;
    std::string content;
    bool is_error = false;
    std::optional<std::string> cache_control;
};

using APIContentBlockParam = std::variant<APIContentBlockText, APIContentBlockImage, APIContentBlockToolResult>;

struct APIMessageParam {
    std::string role;
    std::vector<APIContentBlockParam> content;
};

struct APIToolDefinition {
    std::string name;
    std::string description;
    nlohmann::json input_schema;
};

struct APIToolChoice {
    std::string type;
    std::optional<std::string> name;
};

struct APIThinkingConfig {
    std::string type;
    std::optional<int64_t> budget_tokens;
};

struct APIRequest {
    std::string model;
    std::vector<APIMessageParam> messages;
    std::vector<std::string> system;
    std::vector<APIToolDefinition> tools;
    std::optional<APIToolChoice> tool_choice;
    int64_t max_tokens = 8192;
    std::optional<APIThinkingConfig> thinking;
    std::optional<double> temperature;
    std::optional<std::vector<std::string>> betas;
    bool stream = true;
    std::optional<std::string> metadata_user_id;
};

// ============================================================
// API Response / Stream Event Types
// ============================================================

struct APIUsage {
    int64_t input_tokens = 0;
    int64_t output_tokens = 0;
    int64_t cache_creation_input_tokens = 0;
    int64_t cache_read_input_tokens = 0;
};

struct APIBlockStartText { std::string text; };
struct APIBlockStartToolUse { std::string id; std::string name; std::string input; };
struct APIBlockStartThinking { std::string thinking; std::string signature; };
using APIContentBlockStart = std::variant<APIBlockStartText, APIBlockStartToolUse, APIBlockStartThinking>;

struct APIDeltaText { std::string text; };
struct APIDeltaToolInput { std::string partial_json; };
struct APIDeltaThinking { std::string thinking; };
struct APIDeltaSignature { std::string signature; };
using APIContentBlockDelta = std::variant<APIDeltaText, APIDeltaToolInput, APIDeltaThinking, APIDeltaSignature>;

struct StreamMessageStart {
    std::string id;
    std::string model;
    APIUsage usage;
};

struct StreamContentBlockStart {
    int index;
    APIContentBlockStart block;
};

struct StreamContentBlockDelta {
    int index;
    APIContentBlockDelta delta;
};

struct StreamContentBlockStop {
    int index;
};

struct StreamMessageDelta {
    std::optional<std::string> stop_reason;
    APIUsage usage;
};

struct StreamError {
    std::string error_type;
    std::string message;
};

using APIStreamEvent = std::variant<
    StreamMessageStart,
    StreamContentBlockStart,
    StreamContentBlockDelta,
    StreamContentBlockStop,
    StreamMessageDelta,
    StreamError
>;

// ============================================================
// Error types
// ============================================================

struct APIError {
    int status_code = 0;
    std::string error_type;
    std::string message;
    std::optional<std::string> retry_after;
    bool should_retry = false;
    bool is_overloaded = false;
};

// ============================================================
// JSON serialization
// ============================================================

nlohmann::json api_request_to_json(const APIRequest& req);
APIStreamEvent parse_stream_event(const std::string& event_type, const std::string& data);
APIError parse_error_response(int status_code, const std::string& body);

}  // namespace ccmake
