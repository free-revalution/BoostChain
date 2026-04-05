#pragma once

#include "query/types.hpp"
#include "permissions/permission_manager.hpp"

#include <string>
#include <vector>
#include <unordered_map>

namespace ccmake {

class ToolExecutor {
public:
    struct ToolResult {
        std::string tool_use_id;
        std::string tool_name;
        std::string content;
        bool is_error = false;
        bool permission_denied = false;
    };

    struct PendingTool {
        std::string id;
        std::string name;
        nlohmann::json input;
    };

    // Register a tool by name. Returns false if name already registered.
    bool register_tool(const std::string& name, ToolExecuteFunction fn);

    // Check if a tool is registered
    bool has_tool(const std::string& name) const;

    // Execute tools sequentially, return results in order
    std::vector<ToolResult> execute(const std::vector<PendingTool>& tools);

    // Clear all registered tools
    void clear();

    // Permission management
    void set_permission_manager(PermissionManager* pm);
    PermissionManager* permission_manager() const;

private:
    struct RegisteredTool {
        ToolExecuteFunction fn;
    };
    std::unordered_map<std::string, RegisteredTool> tools_;
    PermissionManager* permission_manager_ = nullptr;
};

}  // namespace ccmake
