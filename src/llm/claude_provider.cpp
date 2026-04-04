#include <boostchain/llm/claude_provider.hpp>
#include <boostchain/error.hpp>
#include <json.hpp>
#include <sstream>

// ClaudeProvider 类的实现
using json = nlohmann::json;

namespace boostchain {

ClaudeProvider::ClaudeProvider(const std::string& api_key,
                               const std::string& base_url)
    : api_key_(api_key)
    , model_("claude-3-5-sonnet-20241022")
    , base_url_(base_url)
    , timeout_seconds_(30) {
    http_client_.set_timeout(timeout_seconds_);
    http_client_.set_header("x-api-key", api_key_);
    http_client_.set_header("anthropic-version", "2023-06-01");
}

// 调用 Claude 模型
ChatResponse ClaudeProvider::chat(const ChatRequest& req) {
    auto result = chat_safe(req);
    if (result.is_error()) {
        throw APIError(result.error_msg());
    }
    return result.unwrap();
}

// 调用 Claude 模型，返回结果
Result<ChatResponse> ClaudeProvider::chat_safe(const ChatRequest& req) {
    try {
        std::string model, base_url;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            model = req.model.empty() ? model_ : req.model;
            base_url = base_url_;
        }

        json request_json;
        request_json["model"] = model;
        request_json["max_tokens"] = req.max_tokens.value_or(4096);

        // Claude separates system messages from the messages array
        json messages_json = json::array();
        std::string system_content;

        for (const auto& msg : req.messages) {
            if (msg.role == Message::system) {
                if (!system_content.empty()) {
                    system_content += "\n\n";
                }
                system_content += msg.content;
            } else {
                json msg_json;
                switch (msg.role) {
                    case Message::user:
                        msg_json["role"] = "user";
                        break;
                    case Message::assistant:
                        msg_json["role"] = "assistant";
                        break;
                    case Message::tool:
                        // Claude doesn't have a direct "tool" role in the same way;
                        // map to user role with tool results
                        msg_json["role"] = "user";
                        break;
                    default:
                        msg_json["role"] = "user";
                        break;
                }
                msg_json["content"] = msg.content;
                messages_json.push_back(msg_json);
            }
        }

        request_json["messages"] = messages_json;
        if (!system_content.empty()) {
            request_json["system"] = system_content;
        }

        if (req.temperature) {
            request_json["temperature"] = *req.temperature;
        }
        if (req.top_p) {
            request_json["top_p"] = *req.top_p;
        }

        std::string url = base_url + "/messages";
        std::string request_body = request_json.dump();

        HttpResponse response = http_client_.post(url, request_body, "application/json");

        if (response.status_code != 200) {
            return Result<ChatResponse>::error(
                "Claude API error: HTTP " + std::to_string(response.status_code) +
                " - " + response.body
            );
        }

        ChatResponse chat_response = parse_response(response.body);
        return Result<ChatResponse>::ok(chat_response);

    } catch (const std::exception& e) {
        return Result<ChatResponse>::error(
            std::string("Exception during Claude chat request: ") + e.what()
        );
    }
}

// 异步调用 Claude 模型
std::future<ChatResponse> ClaudeProvider::chat_async(const ChatRequest& req) {
    return std::async(std::launch::async, [this, req]() {
        return chat(req);
    });
}
// 调用 Claude 模型，返回流式结果

// 流式调用 Claude 模型
void ClaudeProvider::chat_stream(const ChatRequest& req, StreamCallback callback) {
    // TODO: Implement actual SSE streaming
    ChatResponse response = chat(req);
    ChatChunk chunk;
    chunk.id = response.id;
    chunk.object = response.object;
    chunk.created = response.created;
    chunk.model = response.model;
    chunk.delta = response.messages;
    if (response.finish_reason) {
        chunk.finish_reason = response.finish_reason;
    }
    callback(chunk);
}

EmbeddingResponse ClaudeProvider::embed(const EmbeddingRequest& /*req*/) {
    throw APIError("Claude API does not support embeddings");
}

void ClaudeProvider::set_api_key(const std::string& api_key) {
    std::lock_guard<std::mutex> lock(mutex_);
    api_key_ = api_key;
    http_client_.set_header("x-api-key", api_key_);
}

void ClaudeProvider::set_model(const std::string& model) {
    std::lock_guard<std::mutex> lock(mutex_);
    model_ = model;
}

void ClaudeProvider::set_base_url(const std::string& base_url) {
    std::lock_guard<std::mutex> lock(mutex_);
    base_url_ = base_url;
}

void ClaudeProvider::set_timeout(int timeout_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    timeout_seconds_ = timeout_seconds;
    http_client_.set_timeout(timeout_seconds);
}

std::string ClaudeProvider::get_api_key() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return api_key_;
}

std::string ClaudeProvider::get_model() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return model_;
}

std::string ClaudeProvider::get_base_url() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return base_url_;
}

int ClaudeProvider::get_timeout() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return timeout_seconds_;
}

ChatResponse ClaudeProvider::parse_response(const std::string& json_str) {
    try {
        json response_json = json::parse(json_str);

        ChatResponse response;
        response.id = response_json.value("id", "");
        response.object = "message";
        response.created = 0; // Claude doesn't return a unix timestamp
        response.model = response_json.value("model", "");

        if (response_json.contains("usage")) {
            Usage usage;
            usage.prompt_tokens = response_json["usage"].value("input_tokens", 0);
            usage.completion_tokens = response_json["usage"].value("output_tokens", 0);
            usage.total_tokens = usage.prompt_tokens + usage.completion_tokens;
            response.usage = usage;
        }

        if (response_json.contains("content") && !response_json["content"].empty()) {
            for (const auto& block : response_json["content"]) {
                if (block.value("type", "") == "text") {
                    Message msg;
                    msg.role = Message::assistant;
                    msg.content = block.value("text", "");
                    response.messages.push_back(msg);
                }
            }
        }

        response.finish_reason = response_json.value("stop_reason", "");

        return response;

    } catch (const json::exception& e) {
        throw SerializationError("Failed to parse Claude chat response: " + std::string(e.what()));
    }
}

Message::Role ClaudeProvider::parse_role(const std::string& role_str) {
    if (role_str == "user") return Message::user;
    if (role_str == "assistant") return Message::assistant;
    if (role_str == "system") return Message::system;
    if (role_str == "tool") return Message::tool;
    throw SerializationError("Unknown message role: " + role_str);
}

} // namespace boostchain
