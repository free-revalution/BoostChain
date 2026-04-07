#pragma once

#include "tools/tool.hpp"
#include <string>
#include <vector>

namespace ccmake {

struct TodoItem {
    std::string id;
    std::string subject;
    std::string description;
    std::string status;  // "pending", "in_progress", "completed", "deleted"
    std::string created_at;
};

class TodoWriteTool : public ToolBase {
public:
    TodoWriteTool();

    const ToolDefinition& definition() const override;
    std::string validate_input(const nlohmann::json& input, const ToolContext& ctx) const override;
    ToolOutput execute(const nlohmann::json& input, const ToolContext& ctx) override;

    // Access the shared todo list (for testing)
    static std::vector<TodoItem>& todos();
    static void clear_todos();

private:
    static std::string generate_id();
    static std::string current_timestamp();
};

}  // namespace ccmake
