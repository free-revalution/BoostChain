#pragma once

#include <boostchain/llm_provider.hpp>
#include <memory>
#include <string>

namespace boostchain {

class OpenAICompatibleProvider : public ILLMProvider {
public:
    OpenAICompatibleProvider(const std::string& base_url,
                            const std::string& api_key);
    ~OpenAICompatibleProvider();

    ChatResponse chat(const ChatRequest& req) override;
    Result<ChatResponse> chat_safe(const ChatRequest& req) override;
    std::future<ChatResponse> chat_async(const ChatRequest& req) override;
    void chat_stream(const ChatRequest& req,
                    StreamCallback callback) override;
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
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace boostchain
