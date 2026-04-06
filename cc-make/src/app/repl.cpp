#include "app/repl.hpp"
#include "config/config.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>

namespace ccmake {

Repl::Repl(QueryEngine& engine, const CLIArgs& args)
    : engine_(engine), args_(args),
      session_store_(get_global_config_dir() / "sessions"),
      theme_manager_(get_global_config_dir() / "themes") {
    // Register all default commands
    auto cmds = create_default_commands();
    for (auto& cmd : cmds) {
        command_registry_.register_command(cmd);
    }
}

void Repl::set_mcp_manager(void* mgr) {
    // mcp_manager_ = static_cast<McpManager*>(mgr);
    (void)mgr;  // TODO: implement when McpManager is integrated
}

int Repl::run() {
    init_session();
    print_welcome();

    while (running_) {
        auto line = read_line();
        if (line.empty()) continue;

        if (handle_command(line)) continue;

        process_prompt(line);
    }

    return 0;
}

void Repl::init_session() {
    if (!args_.session_id.empty()) {
        resume_session(args_.session_id);
    } else {
        auto id = session_store_.create_session(engine_.model(), ".");
        engine_.set_session_id(id);
        engine_.enable_auto_save(true);
    }
}

bool Repl::resume_session(const std::string& session_id) {
    auto msgs = session_store_.load_session(session_id);
    if (!msgs) {
        std::cerr << "Session not found: " << session_id << "\n";
        return false;
    }

    auto meta = session_store_.load_meta(session_id);
    if (meta) {
        if (!meta->model.empty()) engine_.set_model(meta->model);
    }

    engine_.set_session_id(session_id);
    engine_.enable_auto_save(true);

    // Note: we can't directly set messages_ on QueryEngine (it's private).
    // The session store is used for persistence; for now this sets up
    // auto-save. A full resume would need a load_messages() method.
    std::cout << "Session resumed: " << session_id << " (" << msgs->size() << " messages)\n";
    return true;
}

int Repl::process_prompt(const std::string& prompt) {
    try {
        renderer_.reset();

        auto result = engine_.submit_message(prompt);

        if (result.exit_reason == LoopExitReason::Aborted) {
            std::cout << "\n[Query aborted]\n";
        } else if (result.exit_reason == LoopExitReason::MaxTurns) {
            std::cout << "\n[Max turns reached]\n";
        } else if (result.exit_reason == LoopExitReason::ModelError) {
            std::cout << "\n[Model error: " << result.error_message << "]\n";
        }

        renderer_.on_complete();

        display_response(result);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}

void Repl::print_welcome() {
    std::cout << "cc-make v0.1.0 - C++ Claude Code\n";
    if (!engine_.session_id().empty()) {
        std::cout << "Session: " << engine_.session_id() << "\n";
    }
    std::cout << "Type /help for commands, /quit to exit\n\n";
}

void Repl::print_help() {
    std::cout << "Commands:\n";

    // Built-in commands that need engine access (handled directly in handle_command)
    std::cout << "  /model <name>   Switch model\n"
              << "  /mode <name>    Switch permission mode\n"
              << "  /sessions       List saved sessions\n"
              << "  /resume <id>    Resume a saved session\n"
              << "  /new            Start a new session\n";

    // Commands from the registry
    for (const auto& name : command_registry_.all_names()) {
        auto* cmd = command_registry_.find(name);
        if (cmd) {
            std::cout << "  /" << std::left << std::setw(15) << name
                      << cmd->description() << "\n";
        }
    }
}

std::string Repl::read_line() {
    std::cout << "> " << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) {
        running_ = false;
    }
    return line;
}

bool Repl::handle_command(const std::string& input) {
    if (input.empty() || input[0] != '/') return false;

    // Parse command name and arguments
    auto cmd = input.substr(1);
    auto space = cmd.find(' ');
    std::string cmd_name = (space != std::string::npos) ? cmd.substr(0, space) : cmd;
    std::string cmd_arg = (space != std::string::npos) ? cmd.substr(space + 1) : "";

    // Built-in commands that need direct engine access (handled directly)
    if (cmd_name == "quit" || cmd_name == "exit" || cmd_name == "q") {
        running_ = false;
        return true;
    }

    if (cmd_name == "help" || cmd_name == "h") {
        print_help();
        return true;
    }

    if (cmd_name == "model") {
        if (cmd_arg.empty()) {
            std::cout << "Current model: " << engine_.model() << "\n";
        } else {
            engine_.set_model(cmd_arg);
            std::cout << "Model set to: " << cmd_arg << "\n";
        }
        return true;
    }

    if (cmd_name == "mode") {
        if (cmd_arg.empty()) {
            std::cout << "Usage: /mode <default|acceptEdits|plan|bypass>\n";
        } else {
            auto mode = parse_permission_mode(cmd_arg);
            engine_.set_permission_mode(mode);
            std::cout << "Permission mode set to: " << cmd_arg << "\n";
        }
        return true;
    }

    if (cmd_name == "status") {
        std::cout << "Model: " << engine_.model() << "\n";
        std::cout << "Permission mode: ";
        switch (engine_.permission_manager().mode()) {
            case PermissionMode::Default: std::cout << "default"; break;
            case PermissionMode::AcceptEdits: std::cout << "acceptEdits"; break;
            case PermissionMode::Plan: std::cout << "plan"; break;
            case PermissionMode::BypassPermissions: std::cout << "bypassPermissions"; break;
            case PermissionMode::Auto: std::cout << "auto"; break;
        }
        std::cout << "\nMessages: " << engine_.messages().size() << "\n";
        if (!engine_.session_id().empty()) {
            std::cout << "Session: " << engine_.session_id() << "\n";
        }
        return true;
    }

    if (cmd_name == "sessions") {
        cmd_sessions();
        return true;
    }

    if (cmd_name == "resume") {
        cmd_resume(cmd_arg);
        return true;
    }

    if (cmd_name == "new") {
        cmd_new();
        return true;
    }

    // Delegate to CommandRegistry for all other commands
    CommandContext ctx;
    ctx.args = cmd_arg;
    ctx.engine = &engine_;
    ctx.repl = this;

    if (command_registry_.execute(input, ctx)) {
        return true;
    }

    std::cout << "Unknown command: /" << cmd_name << ". Type /help for commands.\n";
    return true;
}

void Repl::cmd_sessions() {
    auto sessions = session_store_.list_sessions();
    if (sessions.empty()) {
        std::cout << "No saved sessions.\n";
        return;
    }

    std::cout << "Sessions:\n";
    for (const auto& s : sessions) {
        std::cout << "  " << s.id << "  " << s.model
                  << "  " << s.message_count << " messages"
                  << "  " << s.created_at << "\n";
    }
}

void Repl::cmd_resume(const std::string& arg) {
    if (arg.empty()) {
        std::cout << "Usage: /resume <session_id>\n";
        return;
    }
    resume_session(arg);
}

void Repl::cmd_new() {
    auto id = session_store_.create_session(engine_.model(), ".");
    engine_.set_session_id(id);
    engine_.enable_auto_save(true);
    std::cout << "New session: " << id << "\n";
}

void Repl::display_response(const TurnResult& result) {
    for (const auto& msg : result.messages) {
        for (const auto& block : msg.content) {
            if (auto* text = std::get_if<TextBlock>(&block)) {
                if (msg.role == MessageRole::Assistant) {
                    std::cout << text->text << "\n";
                }
            }
        }
    }

    int tool_count = 0;
    int error_count = 0;
    for (const auto& msg : result.messages) {
        for (const auto& block : msg.content) {
            if (auto* tr = std::get_if<ToolResultBlock>(&block)) {
                ++tool_count;
                if (tr->is_error) ++error_count;
            }
        }
    }

    if (tool_count > 0) {
        std::cout << "[" << tool_count << " tool call" << (tool_count > 1 ? "s" : "");
        if (error_count > 0) {
            std::cout << ", " << error_count << " error" << (error_count > 1 ? "s" : "");
        }
        std::cout << "]\n";
    }

    if (result.total_usage.total_input_tokens > 0 || result.total_usage.total_output_tokens > 0) {
        std::cout << "["
                  << result.total_usage.total_input_tokens << " input, "
                  << result.total_usage.total_output_tokens << " output tokens]\n";
    }

    std::cout << "\n";
}

}  // namespace ccmake
