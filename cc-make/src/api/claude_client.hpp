#pragma once

#include "api/types.hpp"
#include "api/http_transport.hpp"
#include "core/types.hpp"
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <mutex>

namespace ccmake {

struct ClaudeClientConfig {
    std::string model;
    std::string system_prompt;
    std::vector<APIToolDefinition> tools;
    int64_t max_tokens = 8192;
    int max_retries = 10;
    std::optional<APIThinkingConfig> thinking;
    std::optional<std::string> api_key;
    std::optional<std::string> base_url;
    int timeout_ms = 600000;
};

// Accumulates state during a streaming session
class StreamSession {
public:
    void on_message_start(const StreamMessageStart& event);
    void on_content_block_start(const StreamContentBlockStart& event);
    void on_content_block_delta(const StreamContentBlockDelta& event);
    void on_content_block_stop(const StreamContentBlockStop& event);
    void on_message_delta(const StreamMessageDelta& event);

    Message build_message() const;

    std::string message_id;
    TokenBudget usage;
    std::vector<ContentBlock> content_blocks;
    std::optional<StopReason> stop_reason;

private:
    std::vector<std::string> tool_input_buffers_;  // accumulated JSON per tool_use block
};

using StreamEventCallback = std::function<void(const APIStreamEvent&)>;
using RetryCallback = std::function<void(int attempt, const std::string& message)>;

class ClaudeClient {
public:
    explicit ClaudeClient(ClaudeClientConfig config);

    Message query(
        const std::vector<Message>& messages,
        StreamEventCallback on_event = nullptr,
        RetryCallback on_retry = nullptr
    );

    Message query_without_streaming(const std::vector<Message>& messages);

    HTTPTransport& transport() { return *transport_; }
    void abort();

private:
    ClaudeClientConfig config_;
    std::unique_ptr<HTTPTransport> transport_;

    Message execute_attempt(
        const std::string& json_body,
        StreamEventCallback on_event
    );

    Message execute_with_retry(
        const std::string& json_body,
        StreamEventCallback on_event,
        RetryCallback on_retry
    );

    std::mutex mutex_;
    bool aborted_ = false;
};

}  // namespace ccmake
