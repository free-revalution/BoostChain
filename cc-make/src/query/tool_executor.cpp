#include "query/tool_executor.hpp"

namespace ccmake {

bool ToolExecutor::register_tool(const std::string& name, ToolExecuteFunction fn) {
    if (tools_.count(name)) return false;
    tools_[name] = {std::move(fn)};
    return true;
}

bool ToolExecutor::has_tool(const std::string& name) const {
    return tools_.count(name) > 0;
}

std::vector<ToolExecutor::ToolResult> ToolExecutor::execute(const std::vector<PendingTool>& tools) {
    std::vector<ToolResult> results;
    results.reserve(tools.size());

    for (const auto& tool : tools) {
        ToolResult result;
        result.tool_use_id = tool.id;
        result.tool_name = tool.name;

        auto it = tools_.find(tool.name);
        if (it == tools_.end()) {
            result.content = "Error: unknown tool '" + tool.name + "'";
            result.is_error = true;
            results.push_back(std::move(result));
            continue;
        }

        // Permission check
        if (permission_manager_) {
            auto input_summary = tool.input.dump();
            auto perm = permission_manager_->check_with_memo(tool.name, input_summary);

            if (perm.behavior == PermissionBehavior::Deny) {
                result.content = "Permission denied: " + perm.message;
                result.is_error = true;
                result.permission_denied = true;
                results.push_back(std::move(result));
                continue;
            }

            if (perm.behavior == PermissionBehavior::Ask) {
                // No callback available — block the execution
                result.content = "Permission required: " + perm.message;
                result.is_error = true;
                result.permission_denied = true;
                results.push_back(std::move(result));
                continue;
            }
        }

        // Execute tool
        try {
            auto output = it->second.fn(tool.name, tool.input);
            result.content = output.dump();
            result.is_error = false;
        } catch (const std::exception& e) {
            result.content = std::string("Error: ") + e.what();
            result.is_error = true;
        }

        results.push_back(std::move(result));
    }

    return results;
}

void ToolExecutor::clear() {
    tools_.clear();
}

void ToolExecutor::copy_tools_from(const ToolExecutor& other) {
    for (const auto& [name, tool] : other.tools_) {
        if (!tools_.count(name)) {
            tools_[name] = tool;
        }
    }
}

void ToolExecutor::set_permission_manager(PermissionManager* pm) {
    permission_manager_ = pm;
}

PermissionManager* ToolExecutor::permission_manager() const {
    return permission_manager_;
}

}  // namespace ccmake
