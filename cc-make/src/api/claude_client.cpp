#include "api/claude_client.hpp"
#include "api/messages.hpp"
#include "api/errors.hpp"
#include "api/sse_parser.hpp"

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <variant>

namespace ccmake {

// ============================================================
// Stop reason mapping
// ============================================================

static std::optional<StopReason> map_stop_reason(const std::string& reason) {
    if (reason == "end_turn") return StopReason::EndTurn;
    if (reason == "max_tokens") return StopReason::MaxTokens;
    if (reason == "tool_use") return StopReason::ToolUse;
    if (reason == "stop_sequence") return StopReason::StopSequence;
    return std::nullopt;
}

// ============================================================
// StreamSession
// ============================================================

void StreamSession::on_message_start(const StreamMessageStart& event) {
    message_id = event.id;
    usage.total_input_tokens = event.usage.input_tokens;
    usage.total_output_tokens = event.usage.output_tokens;
}

void StreamSession::on_content_block_start(const StreamContentBlockStart& event) {
    std::visit([&](const auto& block) {
        using T = std::decay_t<decltype(block)>;
        if constexpr (std::is_same_v<T, APIBlockStartText>) {
            content_blocks.push_back(TextBlock{block.text});
        } else if constexpr (std::is_same_v<T, APIBlockStartToolUse>) {
            content_blocks.push_back(ToolUseBlock{block.id, block.name, nlohmann::json::object()});
            tool_input_buffers_.push_back("");
        } else if constexpr (std::is_same_v<T, APIBlockStartThinking>) {
            content_blocks.push_back(ThinkingBlock{block.thinking, block.signature});
        }
    }, event.block);
}

void StreamSession::on_content_block_delta(const StreamContentBlockDelta& event) {
    if (event.index < 0 || static_cast<size_t>(event.index) >= content_blocks.size()) {
        return;
    }

    size_t idx = static_cast<size_t>(event.index);

    std::visit([&](auto& delta) {
        using T = std::decay_t<decltype(delta)>;
        if constexpr (std::is_same_v<T, APIDeltaText>) {
            auto* text_block = std::get_if<TextBlock>(&content_blocks[idx]);
            if (text_block) {
                text_block->text += delta.text;
            }
        } else if constexpr (std::is_same_v<T, APIDeltaToolInput>) {
            // Find the corresponding tool input buffer index.
            // The index into tool_input_buffers_ is not the same as event.index
            // because not all content blocks are tool_use blocks.
            // We need to count tool_use blocks up to idx.
            size_t tool_idx = 0;
            for (size_t i = 0; i < idx; ++i) {
                if (std::holds_alternative<ToolUseBlock>(content_blocks[i])) {
                    ++tool_idx;
                }
            }
            if (tool_idx < tool_input_buffers_.size()) {
                tool_input_buffers_[tool_idx] += delta.partial_json;
                // Try to parse the accumulated JSON and update the tool block
                auto* tool_block = std::get_if<ToolUseBlock>(&content_blocks[idx]);
                if (tool_block) {
                    auto parsed = nlohmann::json::parse(
                        tool_input_buffers_[tool_idx], nullptr, false
                    );
                    if (!parsed.is_discarded() && parsed.is_object()) {
                        tool_block->input = parsed;
                    }
                }
            }
        } else if constexpr (std::is_same_v<T, APIDeltaThinking>) {
            auto* think_block = std::get_if<ThinkingBlock>(&content_blocks[idx]);
            if (think_block) {
                think_block->thinking += delta.thinking;
            }
        } else if constexpr (std::is_same_v<T, APIDeltaSignature>) {
            auto* think_block = std::get_if<ThinkingBlock>(&content_blocks[idx]);
            if (think_block) {
                think_block->signature = delta.signature;
            }
        }
    }, event.delta);
}

void StreamSession::on_content_block_stop(const StreamContentBlockStop& /*event*/) {
    // No-op: blocks are already accumulated incrementally.
}

void StreamSession::on_message_delta(const StreamMessageDelta& event) {
    if (event.stop_reason.has_value()) {
        stop_reason = map_stop_reason(event.stop_reason.value());
    }
    if (event.usage.output_tokens > 0) {
        usage.total_output_tokens = event.usage.output_tokens;
    }
}

Message StreamSession::build_message() const {
    Message msg;
    msg.id = message_id;
    msg.role = MessageRole::Assistant;
    msg.content = content_blocks;
    msg.stop_reason = stop_reason;
    msg.usage = usage;
    return msg;
}

// ============================================================
// ClaudeClient
// ============================================================

ClaudeClient::ClaudeClient(ClaudeClientConfig config)
    : config_(std::move(config))
{
    HTTPTransportConfig http_config;
    if (config_.base_url.has_value()) {
        http_config.base_url = config_.base_url.value();
    }
    if (config_.api_key.has_value()) {
        http_config.api_key = config_.api_key.value();
    }
    http_config.timeout_ms = config_.timeout_ms;

    transport_ = std::make_unique<HTTPTransport>(std::move(http_config));
}

Message ClaudeClient::execute_attempt(
    const std::string& json_body,
    StreamEventCallback on_event
) {
    SSEParser sse_parser;
    StreamSession session;

    auto response = transport_->post_streaming(
        "/v1/messages",
        json_body,
        [&](const std::string& chunk) {
            auto sse_events = sse_parser.feed(chunk);
            for (auto& sse : sse_events) {
                auto api_event = parse_stream_event(sse.event_type, sse.data);
                if (on_event) {
                    on_event(api_event);
                }
                // Dispatch to StreamSession
                std::visit([&](auto& e) {
                    using T = std::decay_t<decltype(e)>;
                    if constexpr (std::is_same_v<T, StreamMessageStart>) {
                        session.on_message_start(e);
                    } else if constexpr (std::is_same_v<T, StreamContentBlockStart>) {
                        session.on_content_block_start(e);
                    } else if constexpr (std::is_same_v<T, StreamContentBlockDelta>) {
                        session.on_content_block_delta(e);
                    } else if constexpr (std::is_same_v<T, StreamContentBlockStop>) {
                        session.on_content_block_stop(e);
                    } else if constexpr (std::is_same_v<T, StreamMessageDelta>) {
                        session.on_message_delta(e);
                    }
                    // StreamError: not dispatched to session
                }, api_event);
            }
        }
    );

    if (response.status_code == 200) {
        return session.build_message();
    }

    // Parse the error response
    auto error = parse_api_error(response.status_code, response.body);
    throw std::runtime_error(user_facing_error_message(error));
}

Message ClaudeClient::execute_with_retry(
    const std::string& json_body,
    StreamEventCallback on_event,
    RetryCallback on_retry
) {
    for (int attempt = 0; attempt < config_.max_retries; ++attempt) {
        try {
            return execute_attempt(json_body, on_event);
        } catch (const std::exception& e) {
            if (attempt == config_.max_retries - 1) {
                throw;
            }

            // We need an APIError to check retry-ability.
            // For now, extract error info from the exception message and
            // attempt to classify. If we cannot classify, re-throw.
            // The execute_attempt always throws with user_facing_error_message
            // after parse_api_error, so we check should_retry_error on a
            // reconstructed error. However, we don't have the status code here.
            //
            // For a more robust implementation, we could throw a typed exception
            // from execute_attempt. For now, use a simple heuristic: if the
            // attempt is not the last, sleep and retry (the transport may have
            // received a retryable status code).
            std::string error_msg = e.what();

            // Sleep with exponential backoff
            int delay_ms = get_retry_delay_ms(attempt, std::nullopt);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));

            if (on_retry) {
                on_retry(attempt + 1, error_msg);
            }
        }
    }

    throw std::runtime_error("Max retries exceeded");
}

Message ClaudeClient::query(
    const std::vector<Message>& messages,
    StreamEventCallback on_event,
    RetryCallback on_retry
) {
    std::lock_guard<std::mutex> lock(mutex_);
    aborted_ = false;

    auto request = build_api_request(
        config_.model,
        messages,
        config_.system_prompt,
        config_.tools,
        config_.max_tokens,
        config_.thinking
    );

    std::string json_body = api_request_to_json(request).dump();

    return execute_with_retry(json_body, on_event, on_retry);
}

Message ClaudeClient::query_without_streaming(const std::vector<Message>& messages) {
    return query(messages, nullptr, nullptr);
}

void ClaudeClient::abort() {
    std::lock_guard<std::mutex> lock(mutex_);
    aborted_ = true;
    transport_->abort();
}

}  // namespace ccmake
