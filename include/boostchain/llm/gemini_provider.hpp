#pragma once

#include <boostchain/llm_provider.hpp>
#include <boostchain/http_client.hpp>
#include <string>
#include <future>
#include <thread>
#include <mutex>

namespace boostchain {

class GeminiProvider : public ILLMProvider {
public:
    explicit GeminiProvider(const std::string& api_key,
                            const std::string& base_url = "https://generativelanguage.googleapis.com/v1beta");

    ChatResponse chat(const ChatRequest& req) override;
    Result<ChatResponse> chat_safe(const ChatRequest& req) override;
    std::future<ChatResponse> chat_async(const ChatRequest& req) override;
    void chat_stream(const ChatRequest& req, StreamCallback callback) override;
    EmbeddingResponse embed(const EmbeddingRequest& req) override;

    void set_api_key(const std::string& api_key) override;
    void set_model(const std::string& model) override;
    void set_base_url(const std::string& base_url) override;
    void set_timeout(int timeout_seconds) override;

    std::string get_api_key() const override;
    std::string get_model() const override;
    std::string get_base_url() const override;
    int get_timeout() const override;

private:
    HttpClient http_client_;
    std::string api_key_;
    std::string model_;
    std::string base_url_;
    int timeout_seconds_;

    mutable std::mutex mutex_;

    ChatResponse parse_response(const std::string& json_str);
    Message::Role parse_role(const std::string& role_str);
};

} // namespace boostchain
