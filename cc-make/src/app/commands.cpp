#include "app/commands.hpp"
#include "app/repl.hpp"
#include "query/query_engine.hpp"
#include "query/compaction.hpp"
#include "terminal/theme.hpp"
#include "terminal/keybinding.hpp"
#include "git/git.hpp"
#include "config/config.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>

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

    // /help
    cmds.push_back(std::make_shared<SimpleCommand>(
        "help", "Show available commands", std::vector<std::string>{"h"},
        [](const CommandContext&) -> bool {
            std::cout << "Available commands:\n"
                      << "  /help, /h              Show this help message\n"
                      << "  /quit, /exit, /q       Exit the application\n"
                      << "  /clear                  Clear conversation history\n"
                      << "  /compact                Compact conversation context\n"
                      << "  /config [key=value]     View or set configuration\n"
                      << "  /cost                   Show token usage and cost\n"
                      << "  /diff                   Show current git diffs\n"
                      << "  /context                Show context window usage\n"
                      << "  /model [name]           Switch or view current model\n"
                      << "  /mode <name>            Switch permission mode\n"
                      << "  /status                 Show current status\n"
                      << "  /sessions               List saved sessions\n"
                      << "  /resume <id>            Resume a saved session\n"
                      << "  /new                    Start a new session\n"
                      << "  /tools                  List registered tools\n"
                      << "  /theme [name]           Change display theme\n"
                      << "  /keybindings            Show current keybindings\n"
                      << "  /vim                    Toggle vim mode\n"
                      << "  /permissions [list]     Permission management\n"
                      << "  /memory                 Show memory info\n"
                      << "  /undo                   Not yet implemented\n"
                      << "  /bug                    Report a bug\n";
            return true;
        }));

    // /quit
    cmds.push_back(std::make_shared<SimpleCommand>(
        "quit", "Exit the application", std::vector<std::string>{"exit", "q"},
        [](const CommandContext& ctx) -> bool {
            if (ctx.repl) {
                ctx.repl->set_running(false);
            }
            return false;
        }));

    // /clear
    cmds.push_back(std::make_shared<SimpleCommand>(
        "clear", "Clear conversation history", std::vector<std::string>{},
        [](const CommandContext&) -> bool {
            std::cout << "Clear: not yet implemented (requires engine message reset)\n";
            return true;
        },
        /*needs_engine=*/true));

    // /compact
    cmds.push_back(std::make_shared<SimpleCommand>(
        "compact", "Compact conversation context to free space", std::vector<std::string>{},
        [](const CommandContext& ctx) -> bool {
            if (!ctx.engine) {
                std::cout << "Compact: no engine available\n";
                return true;
            }
            const auto& msgs = ctx.engine->messages();
            Compactor compactor;
            auto compacted = compactor.compact(msgs);
            if (static_cast<int>(compacted.size()) < static_cast<int>(msgs.size())) {
                std::cout << "Compacted " << msgs.size() << " messages -> " << compacted.size() << " messages\n";
            } else {
                std::cout << "Conversation is already compact. No changes made.\n";
            }
            return true;
        },
        /*needs_engine=*/true));

    // /config
    cmds.push_back(std::make_shared<SimpleCommand>(
        "config", "View or set configuration", std::vector<std::string>{},
        [](const CommandContext& ctx) -> bool {
            auto config_dir = get_global_config_dir();
            auto config_path = config_dir / "config.yaml";
            auto cfg = Config::load_from_file(config_path);
            if (ctx.args.empty()) {
                // Show current config
                std::cout << "Configuration (" << config_path.string() << "):\n";
                if (cfg) {
                    std::cout << "  model:           " << (cfg->model.empty() ? "(default)" : cfg->model) << "\n";
                    std::cout << "  permission_mode: " << static_cast<int>(cfg->permission_mode) << "\n";
                    std::cout << "  theme:           " << cfg->theme << "\n";
                    std::cout << "  api_key:         " << (cfg->api_key.empty() ? "(not set)" : "***set***") << "\n";
                    std::cout << "  auto_compact:    " << (cfg->auto_compact ? "true" : "false") << "\n";
                    std::cout << "  auto_compact_pct:" << cfg->auto_compact_pct << "%\n";
                    std::cout << "  allow_rules:     " << cfg->allow_rules.size() << "\n";
                    std::cout << "  deny_rules:      " << cfg->deny_rules.size() << "\n";
                } else {
                    std::cout << "  (no config file found, using defaults)\n";
                }
                return true;
            }
            // Parse key=value
            auto eq = ctx.args.find('=');
            if (eq == std::string::npos) {
                std::cout << "Usage: /config [key=value]\n";
                return true;
            }
            std::string key = ctx.args.substr(0, eq);
            std::string value = ctx.args.substr(eq + 1);
            // Trim whitespace
            while (!key.empty() && key.back() == ' ') key.pop_back();
            while (!value.empty() && value.front() == ' ') value.erase(0, 1);

            if (!cfg) cfg = Config{};
            if (key == "model") {
                cfg->model = value;
                if (ctx.engine) ctx.engine->set_model(value);
                std::cout << "Set model to: " << value << "\n";
            } else if (key == "theme") {
                cfg->theme = value;
                std::cout << "Set theme to: " << value << "\n";
            } else if (key == "auto_compact") {
                cfg->auto_compact = (value == "true" || value == "1");
                std::cout << "Set auto_compact to: " << (cfg->auto_compact ? "true" : "false") << "\n";
            } else if (key == "auto_compact_pct") {
                try { cfg->auto_compact_pct = std::stoi(value); }
                catch (...) { std::cout << "Invalid value for auto_compact_pct\n"; return true; }
                std::cout << "Set auto_compact_pct to: " << cfg->auto_compact_pct << "%\n";
            } else if (key == "permission_mode") {
                cfg->permission_mode = parse_permission_mode(value);
                if (ctx.engine) ctx.engine->set_permission_mode(cfg->permission_mode);
                std::cout << "Set permission_mode to: " << value << "\n";
            } else {
                std::cout << "Unknown config key: " << key << "\n";
                std::cout << "Available keys: model, theme, auto_compact, auto_compact_pct, permission_mode\n";
                return true;
            }
            cfg->save_to_file(config_path);
            return true;
        }));

    // /cost
    cmds.push_back(std::make_shared<SimpleCommand>(
        "cost", "Show token usage and cost estimate", std::vector<std::string>{},
        [](const CommandContext& ctx) -> bool {
            if (!ctx.engine) {
                std::cout << "Cost: no engine available\n";
                return true;
            }
            const auto& msgs = ctx.engine->messages();
            // Rough estimate: 4 chars per token
            int total_chars = 0;
            for (const auto& msg : msgs) {
                for (const auto& block : msg.content) {
                    if (auto* text = std::get_if<TextBlock>(&block)) {
                        total_chars += static_cast<int>(text->text.size());
                    }
                }
            }
            int estimated_tokens = total_chars / 4;

            // Claude pricing (approximate per million tokens)
            double input_cost = estimated_tokens * 3.0 / 1000000.0;   // ~$3/MTok input
            double output_cost = estimated_tokens * 15.0 / 1000000.0;  // ~$15/MTok output

            std::cout << "Messages: " << msgs.size() << "\n";
            std::cout << "Estimated tokens: ~" << estimated_tokens << "\n";
            std::cout << "Estimated cost: $" << std::fixed << std::setprecision(4)
                      << (input_cost + output_cost) << "\n";
            std::cout << "  Input:  $" << std::fixed << std::setprecision(4) << input_cost << "\n";
            std::cout << "  Output: $" << std::fixed << std::setprecision(4) << output_cost << "\n";
            return true;
        },
        /*needs_engine=*/true));

    // /diff
    cmds.push_back(std::make_shared<SimpleCommand>(
        "diff", "Show current git diffs", std::vector<std::string>{},
        [](const CommandContext&) -> bool {
            auto status = git_status(".");
            if (!status.is_repo) {
                std::cout << "Not a git repository.\n";
                return true;
            }

            std::cout << "Branch: " << status.branch << "\n";
            if (status.modified.empty() && status.staged.empty() && status.untracked.empty()) {
                std::cout << "No changes.\n";
                return true;
            }

            if (!status.staged.empty()) {
                std::cout << "\nStaged changes:\n";
                for (const auto& f : status.staged) std::cout << "  " << f << "\n";
            }
            if (!status.modified.empty()) {
                std::cout << "\nModified (unstaged):\n";
                for (const auto& f : status.modified) std::cout << "  " << f << "\n";
            }
            if (!status.untracked.empty()) {
                std::cout << "\nUntracked:\n";
                for (const auto& f : status.untracked) std::cout << "  " << f << "\n";
            }

            // Show actual diffs
            auto diffs = git_diff(".");
            if (!diffs.empty()) {
                std::cout << "\n--- Diffs ---\n";
                for (const auto& d : diffs) {
                    std::cout << d.path << " (" << (d.staged ? "staged" : "unstaged") << "):\n";
                    std::cout << d.diff << "\n";
                }
            }
            return true;
        }));

    // /context
    cmds.push_back(std::make_shared<SimpleCommand>(
        "context", "Show context window usage", std::vector<std::string>{},
        [](const CommandContext& ctx) -> bool {
            if (!ctx.engine) {
                std::cout << "Context: no engine available\n";
                return true;
            }
            const auto& msgs = ctx.engine->messages();
            int total_chars = 0;
            for (const auto& msg : msgs) {
                for (const auto& block : msg.content) {
                    if (auto* text = std::get_if<TextBlock>(&block)) {
                        total_chars += static_cast<int>(text->text.size());
                    }
                }
            }
            int estimated_tokens = total_chars / 4;
            int context_window = 200000;

            int pct = (context_window > 0) ? (estimated_tokens * 100 / context_window) : 0;

            std::cout << "Context window: ~" << estimated_tokens << " / " << context_window << " tokens ("
                      << pct << "%)\n";
            std::cout << "Messages: " << msgs.size() << "\n";

            if (pct > 80) {
                std::cout << "WARNING: Context is " << pct << "% full. Consider /compact.\n";
            }
            return true;
        },
        /*needs_engine=*/true));

    // /model
    cmds.push_back(std::make_shared<SimpleCommand>(
        "model", "Switch or view current model", std::vector<std::string>{},
        [](const CommandContext& ctx) -> bool {
            if (!ctx.engine) {
                std::cout << "Model: no engine available\n";
                return true;
            }
            if (ctx.args.empty()) {
                std::cout << "Current model: " << ctx.engine->model() << "\n";
                std::cout << "Available models:\n"
                          << "  claude-sonnet-4-20250514  (default)\n"
                          << "  claude-opus-4-20250514\n"
                          << "  claude-haiku-4-20250514\n";
                return true;
            }
            ctx.engine->set_model(ctx.args);
            std::cout << "Model set to: " << ctx.args << "\n";
            return true;
        },
        /*needs_engine=*/true));

    // /status
    cmds.push_back(std::make_shared<SimpleCommand>(
        "status", "Show current status", std::vector<std::string>{},
        [](const CommandContext& ctx) -> bool {
            std::cout << "cc-make v0.1.0\n\n";

            if (ctx.engine) {
                std::cout << "Model: " << ctx.engine->model() << "\n";
                std::cout << "Permission mode: ";
                switch (ctx.engine->permission_manager().mode()) {
                    case PermissionMode::Default: std::cout << "default"; break;
                    case PermissionMode::AcceptEdits: std::cout << "acceptEdits"; break;
                    case PermissionMode::Plan: std::cout << "plan"; break;
                    case PermissionMode::BypassPermissions: std::cout << "bypassPermissions"; break;
                    case PermissionMode::Auto: std::cout << "auto"; break;
                }
                std::cout << "\n";
                std::cout << "Messages: " << ctx.engine->messages().size() << "\n";
                std::cout << "Tools: " << ctx.engine->tool_registry().all_definitions().size() << " registered\n";

                if (!ctx.engine->session_id().empty()) {
                    std::cout << "Session: " << ctx.engine->session_id() << "\n";
                }
            }

            if (ctx.repl) {
                std::cout << "Vim mode: " << (ctx.repl->vim_mode() ? "enabled" : "disabled") << "\n";
                std::cout << "Theme: " << ctx.repl->theme_manager().current().name << "\n";
            }

            // Git status
            auto git = git_status(".");
            if (git.is_repo) {
                std::cout << "Git branch: " << git.branch << "\n";
                if (git_is_dirty(".")) {
                    std::cout << "Git: " << git.modified.size() << " modified, "
                              << git.staged.size() << " staged, "
                              << git.untracked.size() << " untracked\n";
                } else {
                    std::cout << "Git: clean\n";
                }
            } else {
                std::cout << "Git: not a repository\n";
            }

            return true;
        }));

    // /sessions
    cmds.push_back(std::make_shared<SimpleCommand>(
        "sessions", "List saved sessions", std::vector<std::string>{},
        [](const CommandContext& ctx) -> bool {
            if (!ctx.repl) {
                std::cout << "Sessions: no REPL available\n";
                return true;
            }
            auto sessions = ctx.repl->session_store().list_sessions();
            if (sessions.empty()) {
                std::cout << "No saved sessions.\n";
                return true;
            }
            std::cout << "Sessions:\n";
            for (const auto& s : sessions) {
                std::cout << "  " << s.id << "  " << s.model
                          << "  " << s.message_count << " messages"
                          << "  " << s.created_at << "\n";
            }
            return true;
        }));

    // /resume
    cmds.push_back(std::make_shared<SimpleCommand>(
        "resume", "Resume a saved session", std::vector<std::string>{},
        [](const CommandContext& ctx) -> bool {
            if (!ctx.repl) {
                std::cout << "Resume: no REPL available\n";
                return true;
            }
            if (ctx.args.empty()) {
                std::cout << "Usage: /resume <session_id>\n";
                return true;
            }
            ctx.repl->resume_session(ctx.args);
            return true;
        }));

    // /new
    cmds.push_back(std::make_shared<SimpleCommand>(
        "new", "Start a new session", std::vector<std::string>{},
        [](const CommandContext& ctx) -> bool {
            if (!ctx.repl) {
                std::cout << "New: no REPL available\n";
                return true;
            }
            auto& store = ctx.repl->session_store();
            auto id = store.create_session(ctx.engine ? ctx.engine->model() : "default", ".");
            if (ctx.engine) {
                ctx.engine->set_session_id(id);
                ctx.engine->enable_auto_save(true);
            }
            std::cout << "New session: " << id << "\n";
            return true;
        }));

    // /mode
    cmds.push_back(std::make_shared<SimpleCommand>(
        "mode", "Switch permission mode", std::vector<std::string>{},
        [](const CommandContext& ctx) -> bool {
            if (ctx.args.empty()) {
                std::cout << "Usage: /mode <default|acceptEdits|plan|bypass>\n";
                return true;
            }
            if (ctx.engine) {
                auto mode = parse_permission_mode(ctx.args);
                ctx.engine->set_permission_mode(mode);
                std::cout << "Permission mode set to: " << ctx.args << "\n";
            }
            return true;
        },
        /*needs_engine=*/true));

    // /tools
    cmds.push_back(std::make_shared<SimpleCommand>(
        "tools", "List registered tools", std::vector<std::string>{},
        [](const CommandContext& ctx) -> bool {
            if (!ctx.engine) {
                std::cout << "Tools: no engine available\n";
                return true;
            }
            auto defs = ctx.engine->tool_registry().all_definitions();
            if (defs.empty()) {
                std::cout << "No tools registered.\n";
                return true;
            }
            std::cout << "Registered tools (" << defs.size() << "):\n";
            for (const auto& def : defs) {
                std::cout << "  " << std::left << std::setw(25) << def.name << def.description << "\n";
            }
            return true;
        },
        /*needs_engine=*/true));

    // /theme
    cmds.push_back(std::make_shared<SimpleCommand>(
        "theme", "View or change display theme", std::vector<std::string>{},
        [](const CommandContext& ctx) -> bool {
            if (!ctx.repl) {
                std::cout << "Theme: no REPL available\n";
                return true;
            }
            auto& mgr = ctx.repl->theme_manager();
            if (ctx.args.empty()) {
                std::cout << "Current theme: " << mgr.current().name << "\n";
                auto themes = mgr.available_themes();
                std::cout << "Available themes: ";
                for (size_t i = 0; i < themes.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << themes[i];
                }
                std::cout << "\n";
                return true;
            }
            auto themes = mgr.available_themes();
            bool found = false;
            for (const auto& t : themes) {
                if (t == ctx.args) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                std::cout << "Unknown theme: " << ctx.args << "\n";
                std::cout << "Available themes: ";
                for (size_t i = 0; i < themes.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << themes[i];
                }
                std::cout << "\n";
                return true;
            }
            mgr.set_theme(ctx.args);
            std::cout << "Theme set to: " << ctx.args << "\n";
            return true;
        }));

    // /keybindings
    cmds.push_back(std::make_shared<SimpleCommand>(
        "keybindings", "Show current keybindings", std::vector<std::string>{},
        [](const CommandContext& ctx) -> bool {
            // Use the repl's keybinding manager if available, otherwise create a default one
            KeybindingManager local_mgr;
            KeybindingManager* mgr = &local_mgr;
            if (ctx.repl) {
                mgr = &ctx.repl->keybinding_manager();
            }

            auto json = mgr->to_json();
            if (!json.is_object() || json.empty()) {
                std::cout << "No keybindings configured.\n";
                return true;
            }

            std::cout << "Keybindings:\n";
            for (auto it = json.begin(); it != json.end(); ++it) {
                std::cout << "  " << std::left << std::setw(20) << it.key() << "-> " << it.value() << "\n";
            }
            return true;
        }));

    // /vim
    cmds.push_back(std::make_shared<SimpleCommand>(
        "vim", "Toggle vim mode", std::vector<std::string>{},
        [](const CommandContext& ctx) -> bool {
            if (!ctx.repl) {
                std::cout << "Vim: no REPL available\n";
                return true;
            }
            bool new_mode = !ctx.repl->vim_mode();
            ctx.repl->set_vim_mode(new_mode);
            std::cout << "Vim mode: " << (new_mode ? "enabled" : "disabled") << "\n";
            return true;
        }));

    // /mcp
    cmds.push_back(std::make_shared<SimpleCommand>(
        "mcp", "MCP server management", std::vector<std::string>{},
        [](const CommandContext& ctx) -> bool {
            if (ctx.args.empty() || ctx.args == "list" || ctx.args == "status") {
                std::cout << "MCP servers: (not yet connected)\n";
                std::cout << "Usage: /mcp <list|status|connect|disconnect>\n";
                return true;
            }
            if (ctx.args == "connect") {
                std::cout << "MCP: auto-connect on startup not yet implemented.\n";
                return true;
            }
            std::cout << "MCP: command '" << ctx.args << "' not yet implemented.\n";
            return true;
        }));

    // /permissions
    cmds.push_back(std::make_shared<SimpleCommand>(
        "permissions", "Permission management", std::vector<std::string>{},
        [](const CommandContext& ctx) -> bool {
            if (!ctx.engine) {
                std::cout << "Permissions: no engine available\n";
                return true;
            }
            if (ctx.args.empty() || ctx.args == "list") {
                auto& pm = ctx.engine->permission_manager();
                std::cout << "Permission mode: ";
                switch (pm.mode()) {
                    case PermissionMode::Default: std::cout << "default"; break;
                    case PermissionMode::AcceptEdits: std::cout << "acceptEdits"; break;
                    case PermissionMode::Plan: std::cout << "plan"; break;
                    case PermissionMode::BypassPermissions: std::cout << "bypassPermissions"; break;
                    case PermissionMode::Auto: std::cout << "auto"; break;
                }
                std::cout << "\n";
                const auto& perm_ctx = pm.context();
                std::cout << "Rules: " << perm_ctx.rules.size() << "\n";
                for (const auto& r : perm_ctx.rules) {
                    const char* behavior = "?";
                    if (r.behavior == PermissionBehavior::Allow) behavior = "+";
                    else if (r.behavior == PermissionBehavior::Deny) behavior = "-";
                    std::cout << "  " << behavior << " " << r.tool_name;
                    if (!r.rule_content.empty()) {
                        std::cout << "(" << r.rule_content << ")";
                    }
                    std::cout << " [" << r.source << "]\n";
                }
                return true;
            }
            if (ctx.args.substr(0, 5) == "allow") {
                auto pattern = ctx.args.size() > 6 ? ctx.args.substr(6) : "";
                if (pattern.empty()) {
                    std::cout << "Usage: /permissions allow <tool_pattern>\n";
                    return true;
                }
                ctx.engine->permission_manager().add_allow_rule(pattern);
                std::cout << "Added allow rule: " << pattern << "\n";
                return true;
            }
            if (ctx.args.substr(0, 3) == "deny") {
                auto pattern = ctx.args.size() > 4 ? ctx.args.substr(4) : "";
                if (pattern.empty()) {
                    std::cout << "Usage: /permissions deny <tool_pattern>\n";
                    return true;
                }
                ctx.engine->permission_manager().add_deny_rule(pattern);
                std::cout << "Added deny rule: " << pattern << "\n";
                return true;
            }
            if (ctx.args == "reset") {
                ctx.engine->permission_manager().reset_approvals();
                std::cout << "Reset all memoized permissions.\n";
                return true;
            }
            std::cout << "Usage: /permissions [list|allow|deny|reset]\n";
            return true;
        },
        /*needs_engine=*/true));

    // /memory
    cmds.push_back(std::make_shared<SimpleCommand>(
        "memory", "Show memory information", std::vector<std::string>{},
        [](const CommandContext& ctx) -> bool {
            auto memory_dir = get_global_config_dir();
            auto memory_path = memory_dir / "memory";
            if (std::filesystem::exists(memory_path) && std::filesystem::is_directory(memory_path)) {
                std::vector<std::filesystem::path> entries;
                for (const auto& entry : std::filesystem::directory_iterator(memory_path)) {
                    if (entry.is_regular_file()) entries.push_back(entry.path());
                }
                std::cout << "Memory entries (" << entries.size() << "):\n";
                for (const auto& e : entries) {
                    std::cout << "  " << e.filename().string() << "\n";
                }
            } else {
                std::cout << "Memory: no memory files found.\n";
                std::cout << "Memory directory: " << memory_path.string() << "\n";
            }
            (void)ctx;
            return true;
        }));

    // /undo
    cmds.push_back(std::make_shared<SimpleCommand>(
        "undo", "Undo the last assistant action", std::vector<std::string>{},
        [](const CommandContext&) -> bool {
            std::cout << "Undo: not yet implemented (requires message rollback)\n";
            return true;
        },
        /*needs_engine=*/true));

    // /bug
    cmds.push_back(std::make_shared<SimpleCommand>(
        "bug", "Report a bug", std::vector<std::string>{},
        [](const CommandContext&) -> bool {
            std::cout << "To report a bug:\n"
                      << "  1. Open an issue at: https://github.com/anthropics/claude-code/issues\n"
                      << "  2. Include: OS version, cc-make version, steps to reproduce\n"
                      << "  3. Attach any relevant logs or error output\n";
            return true;
        }));

    return cmds;
}

}  // namespace ccmake
