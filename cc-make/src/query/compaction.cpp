#include "query/compaction.hpp"
#include <algorithm>
#include <sstream>
#include <numeric>

namespace ccmake {

Compactor::Compactor(CompactionConfig config)
    : config_(std::move(config)) {}

bool Compactor::should_compact(const TokenBudget& usage, int message_count) const {
    if (message_count < config_.min_messages) {
        return false;
    }
    int64_t total = usage.total_input_tokens + usage.total_output_tokens;
    double threshold = static_cast<double>(config_.context_window) * config_.trigger_ratio;
    return total > static_cast<int64_t>(threshold);
}

int Compactor::estimate_tokens(const std::string& text) const {
    // Rough estimate: ~4 chars per token
    return static_cast<int>(text.size() / 4);
}

int Compactor::estimate_message_tokens(const Message& msg) const {
    int total = 0;
    for (const auto& block : msg.content) {
        if (auto* tb = std::get_if<TextBlock>(&block)) {
            total += estimate_tokens(tb->text);
        } else if (auto* tu = std::get_if<ToolUseBlock>(&block)) {
            total += estimate_tokens(tu->name);
            total += estimate_tokens(tu->input.dump());
        } else if (auto* tr = std::get_if<ToolResultBlock>(&block)) {
            total += estimate_tokens(tr->content);
        } else if (auto* th = std::get_if<ThinkingBlock>(&block)) {
            total += estimate_tokens(th->thinking);
        }
    }
    return total;
}

std::vector<Message> Compactor::truncate(const std::vector<Message>& messages, int target_tokens) const {
    if (messages.empty()) {
        return {};
    }

    // Always keep system-role messages (first check if any exist as standalone messages)
    // In this codebase, system messages aren't a separate role; system context is embedded.
    // We preserve all messages from the end until target_tokens is exceeded.

    int total_tokens = 0;
    std::vector<Message> result;

    // Iterate from the end, collecting recent messages
    for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
        int msg_tokens = estimate_message_tokens(*it);
        if (total_tokens + msg_tokens > target_tokens && !result.empty()) {
            break;
        }
        result.push_back(*it);
        total_tokens += msg_tokens;
    }

    // Reverse to get original order
    std::reverse(result.begin(), result.end());
    return result;
}

std::vector<Message> Compactor::compact(const std::vector<Message>& messages) const {
    if (static_cast<int>(messages.size()) < config_.min_messages) {
        return messages;
    }

    // Calculate total tokens
    int total_tokens = 0;
    for (const auto& msg : messages) {
        total_tokens += estimate_message_tokens(msg);
    }

    if (total_tokens <= config_.target_tokens) {
        return messages;
    }

    // Separate: keep system-equivalent messages and recent messages, summarize the rest
    int keep_count = std::min(config_.keep_recent, static_cast<int>(messages.size()));

    // System messages: messages where role is neither User nor Assistant are treated as system.
    // In this codebase, there's no system MessageRole, so we just treat the first message
    // as context/system if it's a user message with system-like content.
    // For compaction, we identify "system" messages as those with empty content or special markers.
    std::vector<Message> system_messages;
    std::vector<Message> older_messages;
    std::vector<Message> recent_messages;

    // Split messages: keep last keep_count as recent, rest as older
    int older_end = static_cast<int>(messages.size()) - keep_count;
    for (int i = 0; i < static_cast<int>(messages.size()); ++i) {
        if (i < older_end) {
            older_messages.push_back(messages[i]);
        } else {
            recent_messages.push_back(messages[i]);
        }
    }

    // Create summary message
    int condensed_count = static_cast<int>(older_messages.size());
    std::ostringstream oss;
    oss << "[Previous conversation summary: " << condensed_count << " messages condensed]";

    // Build result: system messages + summary + recent messages
    std::vector<Message> result;
    result.push_back(Message::user("compaction-summary", oss.str()));
    result.insert(result.end(), recent_messages.begin(), recent_messages.end());
    return result;
}

const CompactionConfig& Compactor::config() const {
    return config_;
}

void Compactor::set_config(const CompactionConfig& config) {
    config_ = config;
}

}  // namespace ccmake
