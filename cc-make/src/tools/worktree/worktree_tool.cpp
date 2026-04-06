#include "tools/worktree/worktree_tool.hpp"
#include "git/git.hpp"

#include <sstream>

namespace ccmake {

WorktreeTool::WorktreeTool() = default;

const ToolDefinition& WorktreeTool::definition() const {
    static ToolDefinition def = [] {
        ToolDefinition d;
        d.name = "Worktree";
        d.description = "Manage git worktrees: list, add, or remove worktrees for isolated development.";
        d.input_schema = {
            {"type", "object"},
            {"properties", {
                {"action", {{"type", "string"}, {"description", "The action to perform: list, add, or remove"}}},
                {"path", {{"type", "string"}, {"description", "Path for the worktree (required for add/remove)"}}},
                {"branch", {{"type", "string"}, {"description", "Branch name (required for add)"}}}
            }},
            {"required", {"action"}}
        };
        d.aliases = {"GitWorktree"};
        d.is_read_only = false;
        d.is_destructive = true;
        d.is_concurrency_safe = false;
        d.enabled = true;
        return d;
    }();
    return def;
}

std::string WorktreeTool::validate_input(const nlohmann::json& input, const ToolContext& ctx) const {
    (void)ctx;
    if (!input.contains("action") || !input["action"].is_string()) {
        return "action is required and must be a string";
    }
    auto action = input["action"].get<std::string>();
    if (action != "list" && action != "add" && action != "remove") {
        return "action must be one of: list, add, remove";
    }
    if (action == "add") {
        if (!input.contains("path") || !input["path"].is_string() || input["path"].get<std::string>().empty()) {
            return "path is required for add action";
        }
        if (!input.contains("branch") || !input["branch"].is_string() || input["branch"].get<std::string>().empty()) {
            return "branch is required for add action";
        }
    }
    if (action == "remove") {
        if (!input.contains("path") || !input["path"].is_string() || input["path"].get<std::string>().empty()) {
            return "path is required for remove action";
        }
    }
    return "";
}

ToolOutput WorktreeTool::execute(const nlohmann::json& input, const ToolContext& ctx) {
    if (!input.contains("action") || !input["action"].is_string()) {
        return {"Error: action is required", true};
    }

    auto action = input["action"].get<std::string>();

    if (action == "list") {
        auto result = git_worktree_list(ctx.cwd);
        if (!result.has_value()) {
            return {"Error: " + result.error(), true};
        }

        std::ostringstream out;
        const auto& worktrees = result.value();
        if (worktrees.empty()) {
            out << "No worktrees found.\n";
        } else {
            for (const auto& wt : worktrees) {
                out << wt.path;
                if (!wt.branch.empty()) {
                    out << " [" << wt.branch << "]";
                }
                if (wt.is_main) {
                    out << " (main)";
                }
                out << "\n";
            }
        }
        return {out.str(), false};
    }

    if (action == "add") {
        if (!input.contains("path") || !input["path"].is_string()) {
            return {"Error: path is required for add action", true};
        }
        if (!input.contains("branch") || !input["branch"].is_string()) {
            return {"Error: branch is required for add action", true};
        }
        auto path = input["path"].get<std::string>();
        auto branch = input["branch"].get<std::string>();

        auto result = git_worktree_add(ctx.cwd, path, branch);
        if (!result.has_value()) {
            return {"Error: " + result.error(), true};
        }
        return {"Worktree added at " + path + " on branch " + branch + "\n", false};
    }

    if (action == "remove") {
        if (!input.contains("path") || !input["path"].is_string()) {
            return {"Error: path is required for remove action", true};
        }
        auto path = input["path"].get<std::string>();

        auto result = git_worktree_remove(ctx.cwd, path);
        if (!result.has_value()) {
            return {"Error: " + result.error(), true};
        }
        return {"Worktree removed at " + path + "\n", false};
    }

    return {"Error: unknown action '" + action + "'. Must be one of: list, add, remove", true};
}

}  // namespace ccmake
