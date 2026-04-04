#include "api/messages.hpp"
#include <stdexcept>

namespace ccmake {

// ============================================================
// Internal helpers
// ============================================================

static std::string role_to_string(MessageRole role) {
    switch (role) {
        case MessageRole::User: return "user";
        case MessageRole::Assistant: return "assistant";
    }
    return "user";
}

static nlohmann::json serialize_content_block_text(const TextBlock& block) {
    nlohmann::json j;
    j["type"] = "text";
    j["text"] = block.text;
    return j;
}

static nlohmann::json serialize_content_block_tool_use(const ToolUseBlock& block) {
    nlohmann::json j;
    j["type"] = "tool_use";
    j["id"] = block.id;
    j["name"] = block.name;
    j["input"] = block.input;
    return j;
}

static nlohmann::json serialize_content_block_tool_result(const ToolResultBlock& block) {
    nlohmann::json j;
    j["type"] = "tool_result";
    j["tool_use_id"] = block.tool_use_id;
    j["content"] = block.content;
    j["is_error"] = block.is_error;
    return j;
}

static nlohmann::json serialize_content_block_thinking(const ThinkingBlock& block) {
    nlohmann::json j;
    j["type"] = "thinking";
    j["thinking"] = block.thinking;
    j["signature"] = block.signature;
    return j;
}

static nlohmann::json serialize_content_block(const ContentBlock& block) {
    return std::visit([](const auto& b) -> nlohmann::json {
        using T = std::decay_t<decltype(b)>;
        if constexpr (std::is_same_v<T, TextBlock>) {
            return serialize_content_block_text(b);
        } else if constexpr (std::is_same_v<T, ToolUseBlock>) {
            return serialize_content_block_tool_use(b);
        } else if constexpr (std::is_same_v<T, ToolResultBlock>) {
            return serialize_content_block_tool_result(b);
        } else if constexpr (std::is_same_v<T, ThinkingBlock>) {
            return serialize_content_block_thinking(b);
        }
        return {};
    }, block);
}

// ============================================================
// message_to_api_param
// ============================================================

APIMessageParam message_to_api_param(const Message& msg) {
    APIMessageParam param;
    param.role = role_to_string(msg.role);

    for (const auto& block : msg.content) {
        std::visit([&param](const auto& b) {
            using T = std::decay_t<decltype(b)>;
            if constexpr (std::is_same_v<T, TextBlock>) {
                param.content.emplace_back(APIContentBlockText{b.text, std::nullopt});
            } else if constexpr (std::is_same_v<T, ToolResultBlock>) {
                param.content.emplace_back(APIContentBlockToolResult{
                    b.tool_use_id, b.content, b.is_error, std::nullopt
                });
            }
            // ToolUseBlock and ThinkingBlock are not representable in
            // APIContentBlockParam; they are handled in build_api_request
            // which serializes messages to JSON directly.
        }, block);
    }

    return param;
}

// ============================================================
// build_api_request
// ============================================================

APIRequest build_api_request(
    const std::string& model,
    const std::vector<Message>& messages,
    const std::string& system_prompt,
    const std::vector<APIToolDefinition>& tools,
    int64_t max_tokens,
    const std::optional<APIThinkingConfig>& thinking
) {
    APIRequest req;
    req.model = model;
    req.max_tokens = max_tokens;
    req.stream = true;
    req.thinking = thinking;

    if (!system_prompt.empty()) {
        req.system.push_back(system_prompt);
    }

    req.tools = tools;

    // Convert messages to APIMessageParam format
    for (const auto& msg : messages) {
        req.messages.push_back(message_to_api_param(msg));
    }

    return req;
}

// ============================================================
// api_request_to_json
// ============================================================

nlohmann::json api_request_to_json(const APIRequest& req) {
    nlohmann::json j;
    j["model"] = req.model;
    j["max_tokens"] = req.max_tokens;
    j["stream"] = req.stream;

    // System prompt
    if (!req.system.empty()) {
        j["system"] = req.system;
    }

    // Messages
    j["messages"] = nlohmann::json::array();
    for (const auto& param : req.messages) {
        nlohmann::json msg_json;
        msg_json["role"] = param.role;

        // For "user" role, use the APIContentBlockParam content.
        // For "assistant" role, use the same content array - the JSON
        // serializer produces the right format because we always serialize
        // full content blocks via serialize_content_block in the JSON
        // conversion below.
        msg_json["content"] = nlohmann::json::array();
        for (const auto& block : param.content) {
            std::visit([&msg_json](const auto& b) {
                using T = std::decay_t<decltype(b)>;
                if constexpr (std::is_same_v<T, APIContentBlockText>) {
                    nlohmann::json cb;
                    cb["type"] = "text";
                    cb["text"] = b.text;
                    msg_json["content"].push_back(cb);
                } else if constexpr (std::is_same_v<T, APIContentBlockImage>) {
                    nlohmann::json cb;
                    cb["type"] = "image";
                    cb["source"]["type"] = "base64";
                    cb["source"]["media_type"] = b.media_type;
                    cb["source"]["data"] = b.data;
                    msg_json["content"].push_back(cb);
                } else if constexpr (std::is_same_v<T, APIContentBlockToolResult>) {
                    nlohmann::json cb;
                    cb["type"] = "tool_result";
                    cb["tool_use_id"] = b.tool_use_id;
                    cb["content"] = b.content;
                    cb["is_error"] = b.is_error;
                    msg_json["content"].push_back(cb);
                }
            }, block);
        }

        j["messages"].push_back(msg_json);
    }

    // Tools
    if (!req.tools.empty()) {
        auto& tools_arr = j["tools"];
        for (const auto& tool : req.tools) {
            nlohmann::json tool_json;
            tool_json["name"] = tool.name;
            tool_json["description"] = tool.description;
            tool_json["input_schema"] = tool.input_schema;
            tools_arr.push_back(tool_json);
        }
    }

    // Thinking config
    if (req.thinking.has_value()) {
        auto& think = req.thinking.value();
        nlohmann::json think_json;
        think_json["type"] = think.type;
        if (think.budget_tokens.has_value()) {
            think_json["budget_tokens"] = think.budget_tokens.value();
        }
        j["thinking"] = think_json;
    }

    // Optional fields
    if (req.tool_choice.has_value()) {
        nlohmann::json tc;
        tc["type"] = req.tool_choice->type;
        if (req.tool_choice->name.has_value()) {
            tc["name"] = req.tool_choice->name.value();
        }
        j["tool_choice"] = tc;
    }

    if (req.temperature.has_value()) {
        j["temperature"] = req.temperature.value();
    }

    return j;
}

// ============================================================
// parse_stream_event
// ============================================================

static APIUsage parse_usage(const nlohmann::json& u) {
    APIUsage usage;
    if (u.contains("input_tokens")) usage.input_tokens = u["input_tokens"].get<int64_t>();
    if (u.contains("output_tokens")) usage.output_tokens = u["output_tokens"].get<int64_t>();
    if (u.contains("cache_creation_input_tokens"))
        usage.cache_creation_input_tokens = u["cache_creation_input_tokens"].get<int64_t>();
    if (u.contains("cache_read_input_tokens"))
        usage.cache_read_input_tokens = u["cache_read_input_tokens"].get<int64_t>();
    return usage;
}

APIStreamEvent parse_stream_event(const std::string& event_type, const std::string& data) {
    auto j = nlohmann::json::parse(data, nullptr, false);
    if (j.is_discarded()) {
        return StreamError{"parse_error", "Failed to parse stream event JSON"};
    }

    if (event_type == "message_start") {
        StreamMessageStart evt;
        if (j.contains("message")) {
            const auto& msg = j["message"];
            if (msg.contains("id")) evt.id = msg["id"].get<std::string>();
            if (msg.contains("model")) evt.model = msg["model"].get<std::string>();
            if (msg.contains("usage")) evt.usage = parse_usage(msg["usage"]);
        }
        return evt;
    }

    if (event_type == "content_block_start") {
        StreamContentBlockStart evt;
        if (j.contains("index")) evt.index = j["index"].get<int>();
        if (j.contains("content_block")) {
            const auto& cb = j["content_block"];
            std::string type = cb.value("type", "");
            if (type == "text") {
                evt.block = APIBlockStartText{cb.value("text", "")};
            } else if (type == "tool_use") {
                evt.block = APIBlockStartToolUse{
                    cb.value("id", ""),
                    cb.value("name", ""),
                    cb.value("input", "")
                };
            } else if (type == "thinking") {
                evt.block = APIBlockStartThinking{
                    cb.value("thinking", ""),
                    cb.value("signature", "")
                };
            }
        }
        return evt;
    }

    if (event_type == "content_block_delta") {
        StreamContentBlockDelta evt;
        if (j.contains("index")) evt.index = j["index"].get<int>();
        if (j.contains("delta")) {
            const auto& delta = j["delta"];
            std::string type = delta.value("type", "");
            if (type == "text_delta") {
                evt.delta = APIDeltaText{delta.value("text", "")};
            } else if (type == "input_json_delta") {
                evt.delta = APIDeltaToolInput{delta.value("partial_json", "")};
            } else if (type == "thinking_delta") {
                evt.delta = APIDeltaThinking{delta.value("thinking", "")};
            } else if (type == "signature_delta") {
                evt.delta = APIDeltaSignature{delta.value("signature", "")};
            }
        }
        return evt;
    }

    if (event_type == "content_block_stop") {
        StreamContentBlockStop evt;
        if (j.contains("index")) evt.index = j["index"].get<int>();
        return evt;
    }

    if (event_type == "message_delta") {
        StreamMessageDelta evt;
        if (j.contains("delta")) {
            const auto& delta = j["delta"];
            if (delta.contains("stop_reason")) {
                evt.stop_reason = delta["stop_reason"].get<std::string>();
            }
        }
        if (j.contains("usage")) {
            evt.usage = parse_usage(j["usage"]);
        }
        return evt;
    }

    if (event_type == "error") {
        StreamError evt;
        if (j.contains("error")) {
            const auto& err = j["error"];
            evt.error_type = err.value("type", "unknown_error");
            evt.message = err.value("message", "");
        }
        return evt;
    }

    // Unknown event type - return as error
    return StreamError{"unknown_event", "Unknown event type: " + event_type};
}

// ============================================================
// parse_error_response
// ============================================================

APIError parse_error_response(int status_code, const std::string& body) {
    APIError err;
    err.status_code = status_code;
    err.is_overloaded = (status_code == 529);
    err.should_retry = (status_code == 429 || status_code == 529 || status_code >= 500);

    auto j = nlohmann::json::parse(body, nullptr, false);
    if (j.is_discarded()) {
        err.error_type = "parse_error";
        err.message = "Failed to parse error response JSON";
        return err;
    }

    if (j.contains("error")) {
        const auto& error = j["error"];
        err.error_type = error.value("type", "unknown_error");
        err.message = error.value("message", "");
    }

    // Check for retry-after header hint in the body
    if (j.contains("retry_after")) {
        err.retry_after = j["retry_after"].get<std::string>();
    }

    return err;
}

}  // namespace ccmake
