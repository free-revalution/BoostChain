#include "tools/todo/todo_tool.hpp"

#include <sstream>
#include <chrono>
#include <iomanip>
#include <random>

namespace ccmake {

// Static storage for todos (persists for session)
static std::vector<TodoItem>& todo_storage() {
    static std::vector<TodoItem> todos;
    return todos;
}

std::vector<TodoItem>& TodoWriteTool::todos() {
    return todo_storage();
}

void TodoWriteTool::clear_todos() {
    todo_storage().clear();
}

std::string TodoWriteTool::generate_id() {
    static std::atomic<int> counter{0};
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    std::ostringstream oss;
    oss << "todo-" << now << "-" << counter.fetch_add(1);
    return oss.str();
}

std::string TodoWriteTool::current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

TodoWriteTool::TodoWriteTool() = default;

const ToolDefinition& TodoWriteTool::definition() const {
    static ToolDefinition def = [] {
        ToolDefinition d;
        d.name = "TodoWrite";
        d.description = "Create, update, delete, and list tasks in a todo list for the current session.";
        d.input_schema = {
            {"type", "object"},
            {"properties", {
                {"action", {{"type", "string"}, {"enum", {"create", "update", "delete", "list"}}, {"description", "The action to perform"}}},
                {"subject", {{"type", "string"}, {"description", "Brief title for the task"}}},
                {"description", {{"type", "string"}, {"description", "Detailed description of the task"}}},
                {"status", {{"type", "string"}, {"enum", {"pending", "in_progress", "completed", "deleted"}}, {"description", "Task status"}}},
                {"task_id", {{"type", "string"}, {"description", "ID of the task to update or delete"}}}
            }},
            {"required", {"action"}}
        };
        d.aliases = {"TaskCreate", "TaskUpdate"};
        d.is_read_only = false;
        d.is_destructive = false;
        d.enabled = true;
        return d;
    }();
    return def;
}

std::string TodoWriteTool::validate_input(const nlohmann::json& input, const ToolContext& ctx) const {
    (void)ctx;
    if (!input.contains("action") || !input["action"].is_string()) {
        return "action is required and must be a string";
    }
    auto action = input["action"].get<std::string>();
    if (action != "create" && action != "update" && action != "delete" && action != "list") {
        return "action must be one of: create, update, delete, list";
    }
    if (action == "create" && (!input.contains("subject") || !input["subject"].is_string())) {
        return "subject is required for create action";
    }
    if (action == "update" && (!input.contains("task_id") || !input["task_id"].is_string())) {
        return "task_id is required for update action";
    }
    if (action == "delete" && (!input.contains("task_id") || !input["task_id"].is_string())) {
        return "task_id is required for delete action";
    }
    return "";
}

ToolOutput TodoWriteTool::execute(const nlohmann::json& input, const ToolContext& ctx) {
    (void)ctx;
    auto action = input["action"].get<std::string>();

    if (action == "list") {
        auto& todos = todo_storage();
        if (todos.empty()) {
            return {"No tasks in the todo list.", false};
        }
        std::ostringstream oss;
        int visible_count = 0;
        for (const auto& todo : todos) {
            if (todo.status == "deleted") continue;
            visible_count++;
            oss << "[" << todo.id << "] (" << todo.status << ") " << todo.subject;
            if (!todo.description.empty()) {
                oss << "\n    " << todo.description;
            }
            oss << "\n  Created: " << todo.created_at << "\n\n";
        }
        if (visible_count == 0) {
            return {"No active tasks in the todo list.", false};
        }
        return {oss.str(), false};
    }

    if (action == "create") {
        TodoItem item;
        item.id = generate_id();
        item.subject = input.value("subject", "");
        item.description = input.value("description", "");
        item.status = "pending";
        item.created_at = current_timestamp();
        todo_storage().push_back(std::move(item));

        return {"Task created with ID: " + todo_storage().back().id, false};
    }

    if (action == "update") {
        auto task_id = input["task_id"].get<std::string>();
        auto& todos = todo_storage();
        for (auto& todo : todos) {
            if (todo.id == task_id) {
                if (input.contains("status") && input["status"].is_string()) {
                    todo.status = input["status"].get<std::string>();
                }
                if (input.contains("description") && input["description"].is_string()) {
                    todo.description = input["description"].get<std::string>();
                }
                if (input.contains("subject") && input["subject"].is_string()) {
                    todo.subject = input["subject"].get<std::string>();
                }
                return {"Task " + task_id + " updated.", false};
            }
        }
        return {"Task not found: " + task_id, true};
    }

    if (action == "delete") {
        auto task_id = input["task_id"].get<std::string>();
        auto& todos = todo_storage();
        for (auto& todo : todos) {
            if (todo.id == task_id) {
                todo.status = "deleted";
                return {"Task " + task_id + " deleted.", false};
            }
        }
        return {"Task not found: " + task_id, true};
    }

    return {"Unknown action: " + action, true};
}

}  // namespace ccmake
