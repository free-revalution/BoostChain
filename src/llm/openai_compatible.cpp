#include <boostchain/llm/openai_compatible.hpp>
#include <boostchain/llm/openai_provider.hpp>

namespace boostchain {

// Private implementation class (Pimpl pattern)
class OpenAICompatibleProvider::Impl {
public:
    Impl(const std::string& base_url, const std::string& api_key)
        : openai_provider_(api_key, base_url) {}

    OpenAIProvider openai_provider_;
};

OpenAICompatibleProvider::OpenAICompatibleProvider(const std::string& base_url,
                                                   const std::string& api_key)
    : impl_(std::make_unique<Impl>(base_url, api_key)) {}

OpenAICompatibleProvider::~OpenAICompatibleProvider() = default;

ChatResponse OpenAICompatibleProvider::chat(const ChatRequest& req) {
    return impl_->openai_provider_.chat(req);
}

Result<ChatResponse> OpenAICompatibleProvider::chat_safe(const ChatRequest& req) {
    return impl_->openai_provider_.chat_safe(req);
}

std::future<ChatResponse> OpenAICompatibleProvider::chat_async(const ChatRequest& req) {
    return impl_->openai_provider_.chat_async(req);
}

void OpenAICompatibleProvider::chat_stream(const ChatRequest& req,
                                          StreamCallback callback) {
    impl_->openai_provider_.chat_stream(req, callback);
}

EmbeddingResponse OpenAICompatibleProvider::embed(const EmbeddingRequest& req) {
    return impl_->openai_provider_.embed(req);
}

void OpenAICompatibleProvider::set_api_key(const std::string& api_key) {
    impl_->openai_provider_.set_api_key(api_key);
}

void OpenAICompatibleProvider::set_model(const std::string& model) {
    impl_->openai_provider_.set_model(model);
}

void OpenAICompatibleProvider::set_base_url(const std::string& base_url) {
    impl_->openai_provider_.set_base_url(base_url);
}

void OpenAICompatibleProvider::set_timeout(int timeout_seconds) {
    impl_->openai_provider_.set_timeout(timeout_seconds);
}

std::string OpenAICompatibleProvider::get_api_key() const {
    return impl_->openai_provider_.get_api_key();
}

std::string OpenAICompatibleProvider::get_model() const {
    return impl_->openai_provider_.get_model();
}

std::string OpenAICompatibleProvider::get_base_url() const {
    return impl_->openai_provider_.get_base_url();
}

int OpenAICompatibleProvider::get_timeout() const {
    return impl_->openai_provider_.get_timeout();
}

} // namespace boostchain
