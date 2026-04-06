#include "app/commands.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace ccmake {

// --- SimpleCommand ---

SimpleCommand::SimpleCommand(std::string name, std::string description,
                             std::vector<std::string> aliases,
                             std::function<bool(const CommandContext&)> handler,
                             bool needs_engine)
    : name_(std::move(name)),
      description_(std::move(description)),
      aliases_(std::move(aliases)),
      handler_(std::move(handler)),
      needs_engine_(needs_engine) {}

std::string SimpleCommand::name() const { return name_; }

std::string SimpleCommand::description() const { return description_; }

std::vector<std::string> SimpleCommand::aliases() const { return aliases_; }

bool SimpleCommand::execute(const CommandContext& ctx) {
    if (handler_) {
        return handler_(ctx);
    }
    return true;
}

bool SimpleCommand::needs_engine() const { return needs_engine_; }

// --- CommandRegistry ---

void CommandRegistry::register_command(std::shared_ptr<Command> cmd) {
    if (!cmd) return;
    commands_[cmd->name()] = cmd;
    for (const auto& alias : cmd->aliases()) {
        commands_[alias] = cmd;
    }
}

Command* CommandRegistry::find(const std::string& name) const {
    auto it = commands_.find(name);
    if (it != commands_.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool CommandRegistry::has(const std::string& name) const {
    return find(name) != nullptr;
}

std::vector<std::string> CommandRegistry::all_names() const {
    std::vector<std::string> names;
    for (const auto& [key, cmd] : commands_) {
        if (key == cmd->name()) {
            names.push_back(key);
        }
    }
    return names;
}

bool CommandRegistry::execute(const std::string& input, const CommandContext& ctx) {
    if (input.empty()) return false;

    std::string trimmed = input;
    // Strip leading slash
    if (!trimmed.empty() && trimmed[0] == '/') {
        trimmed = trimmed.substr(1);
    }
    if (trimmed.empty()) return false;

    // First word is command name, rest is args
    std::istringstream iss(trimmed);
    std::string cmd_name;
    iss >> cmd_name;
    if (cmd_name.empty()) return false;

    std::string args;
    // Get the rest of the line after the command name
    if (iss.tellg() != std::istringstream::pos_type(-1)) {
        // Skip the space after the command name
        std::string rest;
        std::getline(iss, rest);
        if (!rest.empty() && rest[0] == ' ') {
            rest = rest.substr(1);
        }
        args = rest;
    }

    Command* cmd = find(cmd_name);
    if (!cmd) return false;

    CommandContext cmd_ctx = ctx;
    cmd_ctx.args = args;
    return cmd->execute(cmd_ctx);
}

void CommandRegistry::remove(const std::string& name) {
    Command* cmd = find(name);
    if (!cmd) return;

    // Erase all entries pointing to this command
    for (auto it = commands_.begin(); it != commands_.end();) {
        if (it->second.get() == cmd) {
            it = commands_.erase(it);
        } else {
            ++it;
        }
    }
}

size_t CommandRegistry::size() const {
    return all_names().size();
}

// --- Built-in commands ---

std::vector<std::shared_ptr<Command>> create_default_commands() {
    std::vector<std::shared_ptr<Command>> cmds;

    cmds.push_back(std::make_shared<SimpleCommand>(
        "help", "Show available commands", std::vector<std::string>{"h"},
        [](const CommandContext&) -> bool {
            std::cout << "Available commands:\n"
                      << "  /help, /h         - Show this help message\n"
                      << "  /quit, /exit, /q  - Exit the application\n"
                      << "  /clear            - Clear conversation state\n"
                      << "  /compact          - Compact conversation context\n"
                      << "  /config           - Configuration settings\n"
                      << "  /cost             - Show token usage and cost\n"
                      << "  /diff             - Show code diffs\n"
                      << "  /context          - Manage context settings\n"
                      << "  /model            - Switch or view current model\n"
                      << "  /status           - Show current status\n"
                      << "  /theme            - Change display theme\n"
                      << "  /keybindings      - Show keybindings\n"
                      << "  /vim              - Toggle vim mode\n"
                      << "  /mcp              - MCP server management\n"
                      << "  /permissions      - Permission management\n"
                      << "  /memory           - Memory management\n";
            return true;
        }));

    cmds.push_back(std::make_shared<SimpleCommand>(
        "quit", "Exit the application", std::vector<std::string>{"exit", "q"},
        [](const CommandContext&) -> bool { return false; }));

    cmds.push_back(std::make_shared<SimpleCommand>(
        "clear", "Clear conversation state", std::vector<std::string>{},
        [](const CommandContext&) -> bool {
            std::cout << "Clear: not yet implemented\n";
            return true;
        }));

    cmds.push_back(std::make_shared<SimpleCommand>(
        "compact", "Compact conversation context", std::vector<std::string>{},
        [](const CommandContext&) -> bool {
            std::cout << "Compact: not yet implemented\n";
            return true;
        },
        /*needs_engine=*/true));

    cmds.push_back(std::make_shared<SimpleCommand>(
        "config", "Configuration settings", std::vector<std::string>{},
        [](const CommandContext&) -> bool {
            std::cout << "Config: not yet implemented\n";
            return true;
        }));

    cmds.push_back(std::make_shared<SimpleCommand>(
        "cost", "Show token usage and cost", std::vector<std::string>{},
        [](const CommandContext&) -> bool {
            std::cout << "Cost: not yet implemented\n";
            return true;
        },
        /*needs_engine=*/true));

    cmds.push_back(std::make_shared<SimpleCommand>(
        "diff", "Show code diffs", std::vector<std::string>{},
        [](const CommandContext&) -> bool {
            std::cout << "Diff: not yet implemented\n";
            return true;
        }));

    cmds.push_back(std::make_shared<SimpleCommand>(
        "context", "Manage context settings", std::vector<std::string>{},
        [](const CommandContext&) -> bool {
            std::cout << "Context: not yet implemented\n";
            return true;
        },
        /*needs_engine=*/true));

    cmds.push_back(std::make_shared<SimpleCommand>(
        "model", "Switch or view current model", std::vector<std::string>{},
        [](const CommandContext&) -> bool {
            std::cout << "Model: not yet implemented\n";
            return true;
        }));

    cmds.push_back(std::make_shared<SimpleCommand>(
        "status", "Show current status", std::vector<std::string>{},
        [](const CommandContext&) -> bool {
            std::cout << "Status: not yet implemented\n";
            return true;
        }));

    cmds.push_back(std::make_shared<SimpleCommand>(
        "theme", "Change display theme", std::vector<std::string>{},
        [](const CommandContext&) -> bool {
            std::cout << "Theme: not yet implemented\n";
            return true;
        }));

    cmds.push_back(std::make_shared<SimpleCommand>(
        "keybindings", "Show keybindings", std::vector<std::string>{},
        [](const CommandContext&) -> bool {
            std::cout << "Keybindings: not yet implemented\n";
            return true;
        }));

    cmds.push_back(std::make_shared<SimpleCommand>(
        "vim", "Toggle vim mode", std::vector<std::string>{},
        [](const CommandContext&) -> bool {
            std::cout << "Vim: not yet implemented\n";
            return true;
        }));

    cmds.push_back(std::make_shared<SimpleCommand>(
        "mcp", "MCP server management", std::vector<std::string>{},
        [](const CommandContext&) -> bool {
            std::cout << "MCP: not yet implemented\n";
            return true;
        }));

    cmds.push_back(std::make_shared<SimpleCommand>(
        "permissions", "Permission management", std::vector<std::string>{},
        [](const CommandContext&) -> bool {
            std::cout << "Permissions: not yet implemented\n";
            return true;
        }));

    cmds.push_back(std::make_shared<SimpleCommand>(
        "memory", "Memory management", std::vector<std::string>{},
        [](const CommandContext&) -> bool {
            std::cout << "Memory: not yet implemented\n";
            return true;
        }));

    return cmds;
}

}  // namespace ccmake
