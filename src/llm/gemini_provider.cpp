#include <boostchain/llm/gemini_provider.hpp>
#include <boostchain/error.hpp>
#include <json.hpp>
#include <sstream>

using json = nlohmann::json;

namespace boostchain {

GeminiProvider::GeminiProvider(const std::string& api_key,
                               const std::string& base_url)
    : api_key_(api_key)
    , model_("gemini-pro")
    , base_url_(base_url)
    , timeout_seconds_(30) {
    http_client_.set_timeout(timeout_seconds_);
}

ChatResponse GeminiProvider::chat(const ChatRequest& req) {
    auto result = chat_safe(req);
    if (result.is_error()) {
        throw APIError(result.error_msg());
    }
    return result.unwrap();
}

Result<ChatResponse> GeminiProvider::chat_safe(const ChatRequest& req) {
    try {
        std::string model, base_url, api_key;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            model = req.model.empty() ? model_ : req.model;
            base_url = base_url_;
            api_key = api_key_;
        }

        json request_json;

        // Gemini uses systemInstruction for system messages
        json contents_json = json::array();
        std::string system_instruction;

        for (const auto& msg : req.messages) {
            if (msg.role == Message::system) {
                if (!system_instruction.empty()) {
                    system_instruction += "\n\n";
                }
                system_instruction += msg.content;
            } else {
                json content_json;
                switch (msg.role) {
                    case Message::user:
                        content_json["role"] = "user";
                        break;
                    case Message::assistant:
                        content_json["role"] = "model";
                        break;
                    case Message::tool:
                        // Map tool role to user for Gemini
                        content_json["role"] = "user";
                        break;
                    default:
                        content_json["role"] = "user";
                        break;
                }
                content_json["parts"] = json::array();
                content_json["parts"].push_back({{"text", msg.content}});
                contents_json.push_back(content_json);
            }
        }

        request_json["contents"] = contents_json;

        if (!system_instruction.empty()) {
            request_json["systemInstruction"] = {{"parts", json::array({{{"text", system_instruction}}})}};
        }

        // Gemini generation config
        json gen_config = json::object();
        if (req.temperature) {
            gen_config["temperature"] = *req.temperature;
        }
        if (req.top_p) {
            gen_config["topP"] = *req.top_p;
        }
        if (req.max_tokens) {
            gen_config["maxOutputTokens"] = *req.max_tokens;
        }
        if (req.stop && !req.stop->empty()) {
            gen_config["stopSequences"] = *req.stop;
        }
        if (!gen_config.empty()) {
            request_json["generationConfig"] = gen_config;
        }

        // URL format: .../models/{model}:generateContent?key={api_key}
        std::string url = base_url + "/models/" + model + ":generateContent?key=" + api_key;
        std::string request_body = request_json.dump();

        HttpResponse response = http_client_.post(url, request_body, "application/json");

        if (response.status_code != 200) {
            return Result<ChatResponse>::error(
                "Gemini API error: HTTP " + std::to_string(response.status_code) +
                " - " + response.body
            );
        }

        ChatResponse chat_response = parse_response(response.body);
        chat_response.model = model;
        return Result<ChatResponse>::ok(chat_response);

    } catch (const std::exception& e) {
        return Result<ChatResponse>::error(
            std::string("Exception during Gemini chat request: ") + e.what()
        );
    }
}

std::future<ChatResponse> GeminiProvider::chat_async(const ChatRequest& req) {
    return std::async(std::launch::async, [this, req]() {
        return chat(req);
    });
}

void GeminiProvider::chat_stream(const ChatRequest& req, StreamCallback callback) {
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

EmbeddingResponse GeminiProvider::embed(const EmbeddingRequest& req) {
    try {
        std::string model, base_url, api_key;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            model = req.model.empty() ? model_ : req.model;
            base_url = base_url_;
            api_key = api_key_;
        }

        json request_json;
        for (const auto& input : req.inputs) {
            request_json["requests"].push_back({{"model", "models/" + model}, {"content", input}});
        }

        if (req.encoding_format) {
            request_json["encodingFormat"] = *req.encoding_format;
        }

        std::string url = base_url + "/models/" + model + ":batchEmbedContents?key=" + api_key;
        std::string request_body = request_json.dump();

        HttpResponse response = http_client_.post(url, request_body, "application/json");

        if (response.status_code != 200) {
            throw APIError(
                "Gemini API error: HTTP " + std::to_string(response.status_code) +
                " - " + response.body
            );
        }

        json response_json = json::parse(response.body);

        EmbeddingResponse embedding_response;
        embedding_response.object = "list";
        embedding_response.model = model;

        if (response_json.contains("embeddings")) {
            int index = 0;
            for (const auto& item : response_json["embeddings"]) {
                Embedding emb;
                emb.object = "embedding";
                emb.index = index++;

                if (item.contains("values")) {
                    std::vector<float> values = item["values"];
                    json emb_json = values;
                    emb.embedding = emb_json.dump();
                }

                embedding_response.data.push_back(emb);
            }
        }

        return embedding_response;

    } catch (const json::exception& e) {
        throw SerializationError("Failed to parse Gemini embedding response: " + std::string(e.what()));
    } catch (const std::exception& e) {
        throw APIError(std::string("Gemini embedding request failed: ") + e.what());
    }
}

void GeminiProvider::set_api_key(const std::string& api_key) {
    std::lock_guard<std::mutex> lock(mutex_);
    api_key_ = api_key;
}

void GeminiProvider::set_model(const std::string& model) {
    std::lock_guard<std::mutex> lock(mutex_);
    model_ = model;
}

void GeminiProvider::set_base_url(const std::string& base_url) {
    std::lock_guard<std::mutex> lock(mutex_);
    base_url_ = base_url;
}

void GeminiProvider::set_timeout(int timeout_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    timeout_seconds_ = timeout_seconds;
    http_client_.set_timeout(timeout_seconds);
}

std::string GeminiProvider::get_api_key() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return api_key_;
}

std::string GeminiProvider::get_model() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return model_;
}

std::string GeminiProvider::get_base_url() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return base_url_;
}

int GeminiProvider::get_timeout() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return timeout_seconds_;
}

ChatResponse GeminiProvider::parse_response(const std::string& json_str) {
    try {
        json response_json = json::parse(json_str);

        ChatResponse response;
        response.id = response_json.value("modelVersion", "");
        response.object = "generateContentResponse";
        response.created = 0; // Gemini doesn't return a unix timestamp
        response.model = response_json.value("modelVersion", "");

        // Parse candidates
        if (response_json.contains("candidates") && !response_json["candidates"].empty()) {
            const auto& candidate = response_json["candidates"][0];

            if (candidate.contains("finishReason")) {
                response.finish_reason = candidate["finishReason"];
            }

            if (candidate.contains("content")) {
                const auto& content = candidate["content"];
                std::string role_str = content.value("role", "model");
                Message msg;
                msg.role = parse_role(role_str);

                if (content.contains("parts") && !content["parts"].empty()) {
                    for (const auto& part : content["parts"]) {
                        if (part.contains("text")) {
                            msg.content += part["text"].get<std::string>();
                        }
                    }
                }

                response.messages.push_back(msg);
            }
        }

        // Parse usage metadata
        if (response_json.contains("usageMetadata")) {
            const auto& usage_json = response_json["usageMetadata"];
            Usage usage;
            usage.prompt_tokens = usage_json.value("promptTokenCount", 0);
            usage.completion_tokens = usage_json.value("candidatesTokenCount", 0);
            usage.total_tokens = usage_json.value("totalTokenCount", 0);
            response.usage = usage;
        }

        return response;

    } catch (const json::exception& e) {
        throw SerializationError("Failed to parse Gemini chat response: " + std::string(e.what()));
    }
}

Message::Role GeminiProvider::parse_role(const std::string& role_str) {
    if (role_str == "user") return Message::user;
    if (role_str == "model") return Message::assistant;
    if (role_str == "system") return Message::system;
    if (role_str == "tool") return Message::tool;
    throw SerializationError("Unknown message role: " + role_str);
}

} // namespace boostchain
