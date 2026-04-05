#pragma once

#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>

namespace ccmake {

// ============================================================
// Tool Definition (schema + metadata)
// ============================================================

struct ToolDefinition {
    std::string name;                                    // e.g. "Read", "Edit", "Write"
    std::string description;                            // Human-readable description
    nlohmann::json input_schema;                         // JSON Schema for input params
    std::vector<std::string> aliases;                   // Alternative names
    bool is_read_only = false;
    bool is_destructive = false;
    bool is_concurrency_safe = false;
    bool enabled = true;
};

// ============================================================
// Tool Execution Context
// ============================================================

struct ToolContext {
    std::string cwd;                    // Current working directory
    bool* abort_flag = nullptr;         // Pointer to abort signal
    void* app_state = nullptr;          // Opaque pointer to app state (future use)
};

// ============================================================
// Tool Result
// ============================================================

struct ToolOutput {
    std::string content;                // Text content to send back to the model
    bool is_error = false;
};

// ============================================================
// Tool Base — interface all tools implement
// ============================================================

class ToolBase {
public:
    virtual ~ToolBase() = default;

    // Get the tool definition (name, schema, metadata)
    virtual const ToolDefinition& definition() const = 0;

    // Validate input against schema and tool-specific rules
    // Returns empty string on success, error message on failure
    virtual std::string validate_input(const nlohmann::json& input, const ToolContext& ctx) const {
        (void)input; (void)ctx;
        return "";
    }

    // Execute the tool with validated input
    virtual ToolOutput execute(const nlohmann::json& input, const ToolContext& ctx) = 0;
};

// Convenience: build a ToolDefinition
inline ToolDefinition make_tool_def(
    std::string name,
    std::string description,
    nlohmann::json input_schema,
    std::vector<std::string> aliases = {},
    bool read_only = false,
    bool destructive = false,
    bool concurrency_safe = false
) {
    return ToolDefinition{
        std::move(name),
        std::move(description),
        std::move(input_schema),
        std::move(aliases),
        read_only,
        destructive,
        concurrency_safe
    };
}

}  // namespace ccmake
