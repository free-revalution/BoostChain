#pragma once

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <chrono>
#include <cstdint>

#include <nlohmann/json.hpp>

namespace ccmake {

enum class PermissionBehavior { Allow, Deny, Ask, Passthrough };
enum class PermissionMode { Default, AcceptEdits, Plan, BypassPermissions, Auto };
enum class APIProvider { FirstParty, Bedrock, Vertex, Foundry };
enum class MessageRole { User, Assistant };
enum class ContentBlockType { Text, ToolUse, ToolResult, Thinking, Image };
enum class StopReason { EndTurn, MaxTokens, ToolUse, StopSequence };
enum class Screen { Prompt, Transcript };
enum class InputMode { Prompt, Search, Edit, PlanModeInput };
enum class ModelTier { SmallFast, Standard, Large };
enum class StreamEventType { AssistantMessage, ToolProgress, Terminal, Compaction };

struct TokenBudget {
    int64_t total_input_tokens = 0;
    int64_t total_output_tokens = 0;
    int64_t context_window = 200000;
};

struct FileLocation {
    std::string path;
    int line = 0;
    int column = 0;
};

struct TextBlock { std::string text; };
struct ThinkingBlock { std::string thinking; std::string signature; };

struct ToolUseBlock {
    std::string id;
    std::string name;
    nlohmann::json input;
};

struct ToolResultBlock {
    std::string tool_use_id;
    std::string content;
    bool is_error = false;
};

using ContentBlock = std::variant<TextBlock, ToolUseBlock, ToolResultBlock, ThinkingBlock>;

struct Message {
    std::string id;
    MessageRole role;
    std::vector<ContentBlock> content;
    std::optional<StopReason> stop_reason;
    TokenBudget usage;

    static Message user(std::string id, std::string text);
    static Message assistant(std::string id, std::vector<ContentBlock> content,
                             std::optional<StopReason> stop = std::nullopt);
};

struct StreamEvent {
    StreamEventType type;
    std::variant<Message, std::string> data;
};

}  // namespace ccmake
