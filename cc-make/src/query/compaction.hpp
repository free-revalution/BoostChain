#pragma once

#include <string>
#include <vector>
#include "core/types.hpp"

namespace ccmake {

struct CompactionConfig {
    int context_window = 200000;       // total context window in tokens
    double trigger_ratio = 0.8;        // compact when usage exceeds this ratio
    int target_tokens = 50000;         // target size after compaction
    int min_messages = 4;              // don't compact if fewer messages
    int keep_recent = 4;               // always keep this many recent messages
};

class Compactor {
public:
    explicit Compactor(CompactionConfig config = {});

    // Check if compaction is needed
    bool should_compact(const TokenBudget& usage, int message_count) const;

    // Truncate message history to fit within target_tokens (rough estimate)
    std::vector<Message> truncate(const std::vector<Message>& messages, int target_tokens) const;

    // Full compaction: summarize old messages, keep recent ones
    std::vector<Message> compact(const std::vector<Message>& messages) const;

    const CompactionConfig& config() const;
    void set_config(const CompactionConfig& config);

private:
    // Rough token estimation: ~4 chars per token
    int estimate_tokens(const std::string& text) const;

    // Estimate total tokens for a message's content blocks
    int estimate_message_tokens(const Message& msg) const;

    CompactionConfig config_;
};

}  // namespace ccmake
