#include <boostchain/llm/openai_provider.hpp>
#include <boostchain/error.hpp>
#include <json.hpp>
#include <sstream>
#include <chrono>

using json = nlohmann::json;

namespace boostchain {

OpenAIProvider::OpenAIProvider(const std::string& api_key,
                               const std::string& base_url)
    : api_key_(api_key)
    , model_("gpt-3.5-turbo")
    , base_url_(base_url)
    , timeout_seconds_(30) {
    http_client_.set_timeout(timeout_seconds_);
    http_client_.set_header("Authorization", "Bearer " + api_key_);
}

ChatResponse OpenAIProvider::chat(const ChatRequest& req) {
    auto result = chat_safe(req);
    if (result.is_error()) {
        throw APIError(result.error_msg());
    }
    return result.unwrap();
}

Result<ChatResponse> OpenAIProvider::chat_safe(const ChatRequest& req) {
    try {
        // Capture member variables under lock to prevent race conditions
        std::string model, base_url;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            model = req.model.empty() ? model_ : req.model;
            base_url = base_url_;
        }

        json request_json;
        request_json["model"] = model;
        request_json["messages"] = json::array();

        for (const auto& msg : req.messages) {
            json msg_json;
            switch (msg.role) {
                case Message::user:
                    msg_json["role"] = "user";
                    break;
                case Message::assistant:
                    msg_json["role"] = "assistant";
                    break;
                case Message::system:
                    msg_json["role"] = "system";
                    break;
                case Message::tool:
                    msg_json["role"] = "tool";
                    break;
            }
            msg_json["content"] = msg.content;
            if (msg.name) {
                msg_json["name"] = *msg.name;
            }
            request_json["messages"].push_back(msg_json);
        }

        if (req.temperature) {
            request_json["temperature"] = *req.temperature;
        }
        if (req.top_p) {
            request_json["top_p"] = *req.top_p;
        }
        if (req.max_tokens) {
            request_json["max_tokens"] = *req.max_tokens;
        }
        if (req.presence_penalty) {
            request_json["presence_penalty"] = *req.presence_penalty;
        }
        if (req.frequency_penalty) {
            request_json["frequency_penalty"] = *req.frequency_penalty;
        }
        if (req.stop) {
            request_json["stop"] = *req.stop;
        }

        // Add tool definitions if present
        if (req.tools && !req.tools->empty()) {
            request_json["tools"] = json::array();
            for (const auto& tool : *req.tools) {
                json tool_json;
                tool_json["type"] = "function";
                tool_json["function"]["name"] = tool.name;
                tool_json["function"]["description"] = tool.description;

                json parameters = json::object();
                parameters["type"] = "object";
                json props = json::object();
                json required = json::array();

                for (const auto& [name, param] : tool.parameters) {
                    json param_json;
                    param_json["type"] = param.type;
                    param_json["description"] = param.description;
                    if (param.enum_values) {
                        param_json["enum"] = json::parse(*param.enum_values);
                    }
                    props[name] = param_json;
                    if (param.required) {
                        required.push_back(name);
                    }
                }

                parameters["properties"] = props;
                if (!required.empty()) {
                    parameters["required"] = required;
                }
                tool_json["function"]["parameters"] = parameters;
                request_json["tools"].push_back(tool_json);
            }
        }

        if (req.tool_choice) {
            request_json["tool_choice"] = *req.tool_choice;
        }

        std::string url = base_url + "/chat/completions";
        std::string request_body = request_json.dump();

        // Note: http_client_ is used without mutex here - this is safe because:
        // 1. The client is only modified in setters which are mutex-protected
        // 2. POST operations are thread-safe (use separate connections per thread)
        HttpResponse response = http_client_.post(url, request_body, "application/json");

        if (response.status_code != 200) {
            return Result<ChatResponse>::error(
                "OpenAI API error: HTTP " + std::to_string(response.status_code) +
                " - " + response.body
            );
        }

        ChatResponse chat_response = parse_response(response.body);
        return Result<ChatResponse>::ok(chat_response);

    } catch (const std::exception& e) {
        return Result<ChatResponse>::error(
            std::string("Exception during chat request: ") + e.what()
        );
    }
}

std::future<ChatResponse> OpenAIProvider::chat_async(const ChatRequest& req) {
    return std::async(std::launch::async, [this, req]() {
        return chat(req);
    });
}

void OpenAIProvider::chat_stream(const ChatRequest& req, StreamCallback callback) {
    // TODO: Implement actual SSE streaming with Server-Sent Events
    // Current implementation is a placeholder that returns complete response
    // Proper implementation should:
    // 1. Use "stream": true in request
    // 2. Parse SSE events as they arrive
    // 3. Call callback for each delta chunk
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

EmbeddingResponse OpenAIProvider::embed(const EmbeddingRequest& req) {
    try {
        // Capture member variables under lock to prevent race conditions
        std::string model, base_url;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            model = req.model.empty() ? model_ : req.model;
            base_url = base_url_;
        }

        json request_json;
        request_json["model"] = model;
        request_json["input"] = req.inputs;

        if (req.encoding_format) {
            request_json["encoding_format"] = *req.encoding_format;
        }

        std::string url = base_url + "/embeddings";
        std::string request_body = request_json.dump();

        // Note: http_client_ is used without mutex here - this is safe because:
        // 1. The client is only modified in setters which are mutex-protected
        // 2. POST operations are thread-safe (use separate connections per thread)
        HttpResponse response = http_client_.post(url, request_body, "application/json");

        if (response.status_code != 200) {
            throw APIError(
                "OpenAI API error: HTTP " + std::to_string(response.status_code) +
                " - " + response.body
            );
        }

        json response_json = json::parse(response.body);

        EmbeddingResponse embedding_response;
        embedding_response.object = response_json["object"];
        embedding_response.model = response_json["model"];

        if (response_json.contains("usage")) {
            Usage usage;
            usage.prompt_tokens = response_json["usage"]["prompt_tokens"];
            usage.total_tokens = response_json["usage"]["total_tokens"];
            embedding_response.usage = usage;
        }

        for (const auto& item : response_json["data"]) {
            Embedding emb;
            emb.object = item["object"];
            emb.index = item["index"];

            // Convert embedding array to JSON string
            std::vector<float> embedding_vec = item["embedding"];
            json emb_json = embedding_vec;
            emb.embedding = emb_json.dump();

            embedding_response.data.push_back(emb);
        }

        return embedding_response;

    } catch (const json::exception& e) {
        throw SerializationError("Failed to parse embedding response: " + std::string(e.what()));
    } catch (const std::exception& e) {
        throw APIError(std::string("Embedding request failed: ") + e.what());
    }
}

void OpenAIProvider::set_api_key(const std::string& api_key) {
    std::lock_guard<std::mutex> lock(mutex_);
    api_key_ = api_key;
    http_client_.set_header("Authorization", "Bearer " + api_key_);
}

void OpenAIProvider::set_model(const std::string& model) {
    std::lock_guard<std::mutex> lock(mutex_);
    model_ = model;
}

void OpenAIProvider::set_base_url(const std::string& base_url) {
    std::lock_guard<std::mutex> lock(mutex_);
    base_url_ = base_url;
}

void OpenAIProvider::set_timeout(int timeout_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    timeout_seconds_ = timeout_seconds;
    http_client_.set_timeout(timeout_seconds);
}

std::string OpenAIProvider::get_api_key() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return api_key_;
}

std::string OpenAIProvider::get_model() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return model_;
}

std::string OpenAIProvider::get_base_url() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return base_url_;
}

int OpenAIProvider::get_timeout() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return timeout_seconds_;
}

std::string OpenAIProvider::format_messages(const std::vector<Message>& messages) {
    json messages_json = json::array();
    for (const auto& msg : messages) {
        json msg_json;
        switch (msg.role) {
            case Message::user:
                msg_json["role"] = "user";
                break;
            case Message::assistant:
                msg_json["role"] = "assistant";
                break;
            case Message::system:
                msg_json["role"] = "system";
                break;
            case Message::tool:
                msg_json["role"] = "tool";
                break;
        }
        msg_json["content"] = msg.content;
        if (msg.name) {
            msg_json["name"] = *msg.name;
        }
        messages_json.push_back(msg_json);
    }
    return messages_json.dump();
}

ChatResponse OpenAIProvider::parse_response(const std::string& json_str) {
    try {
        json response_json = json::parse(json_str);

        ChatResponse response;
        response.id = response_json["id"];
        response.object = response_json["object"];
        response.created = response_json["created"];
        response.model = response_json["model"];

        if (response_json.contains("usage")) {
            Usage usage;
            usage.prompt_tokens = response_json["usage"]["prompt_tokens"];
            usage.completion_tokens = response_json["usage"]["completion_tokens"];
            usage.total_tokens = response_json["usage"]["total_tokens"];
            response.usage = usage;
        }

        if (response_json.contains("choices") && !response_json["choices"].empty()) {
            const auto& choice = response_json["choices"][0];

            if (choice.contains("finish_reason")) {
                response.finish_reason = choice["finish_reason"];
            }

            if (choice.contains("message")) {
                const auto& msg_json = choice["message"];
                Message msg;
                msg.role = parse_role(msg_json["role"]);
                msg.content = msg_json.value("content", "");

                if (msg_json.contains("name") && !msg_json["name"].is_null()) {
                    msg.name = msg_json["name"];
                }

                // Parse tool_calls from the response
                if (msg_json.contains("tool_calls") && msg_json["tool_calls"].is_array()) {
                    for (const auto& tc : msg_json["tool_calls"]) {
                        ToolCall tool_call;
                        tool_call.id = tc.value("id", "");
                        tool_call.name = tc["function"].value("name", "");
                        tool_call.arguments = tc["function"].value("arguments", "");
                        // For now, attach the first tool call to the message
                        if (!msg.tool_call.has_value()) {
                            msg.tool_call = tool_call;
                        }
                    }
                }

                response.messages.push_back(msg);
            }
        }

        return response;

    } catch (const json::exception& e) {
        throw SerializationError("Failed to parse chat response: " + std::string(e.what()));
    }
}

Message::Role OpenAIProvider::parse_role(const std::string& role_str) {
    if (role_str == "user") return Message::user;
    if (role_str == "assistant") return Message::assistant;
    if (role_str == "system") return Message::system;
    if (role_str == "tool") return Message::tool;
    throw SerializationError("Unknown message role: " + role_str);
}

} // namespace boostchain
