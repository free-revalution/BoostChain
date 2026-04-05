#include "app/repl.hpp"

#include <iostream>
#include <sstream>

namespace ccmake {

Repl::Repl(QueryEngine& engine, const CLIArgs& args)
    : engine_(engine), args_(args) {}

int Repl::run() {
    print_welcome();

    while (running_) {
        auto line = read_line();
        if (line.empty()) continue;

        if (handle_command(line)) continue;

        process_prompt(line);
    }

    return 0;
}

int Repl::process_prompt(const std::string& prompt) {
    try {
        auto result = engine_.submit_message(prompt);

        if (result.exit_reason == LoopExitReason::Aborted) {
            std::cout << "\n[Query aborted]\n";
        } else if (result.exit_reason == LoopExitReason::MaxTurns) {
            std::cout << "\n[Max turns reached]\n";
        } else if (result.exit_reason == LoopExitReason::ModelError) {
            std::cout << "\n[Model error: " << result.error_message << "]\n";
        }

        display_response(result);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}

void Repl::print_welcome() {
    std::cout << "cc-make v0.1.0 - C++ Claude Code\n";
    std::cout << "Type /help for commands, /quit to exit\n\n";
}

void Repl::print_help() {
    std::cout << "Commands:\n"
              << "  /help          Show this help\n"
              << "  /quit          Exit cc-make\n"
              << "  /model <name>  Switch model\n"
              << "  /mode <name>   Switch permission mode (default, acceptEdits, plan, bypass)\n"
              << "  /compact       Compact conversation\n"
              << "  /clear         Clear conversation history\n"
              << "  /status        Show current status\n";
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

    auto cmd = input.substr(1);
    auto space = cmd.find(' ');
    std::string cmd_name = (space != std::string::npos) ? cmd.substr(0, space) : cmd;
    std::string cmd_arg = (space != std::string::npos) ? cmd.substr(space + 1) : "";

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

    if (cmd_name == "clear") {
        std::cout << "Conversation cleared.\n";
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
        return true;
    }

    if (cmd_name == "compact") {
        std::cout << "Compact not yet implemented.\n";
        return true;
    }

    std::cout << "Unknown command: /" << cmd_name << ". Type /help for commands.\n";
    return true;
}

void Repl::display_response(const TurnResult& result) {
    // Find the last text block in the messages and display it
    for (const auto& msg : result.messages) {
        for (const auto& block : msg.content) {
            if (auto* text = std::get_if<TextBlock>(&block)) {
                // Only display text blocks from the latest assistant response
                // (skip intermediate tool results)
                if (msg.role == MessageRole::Assistant) {
                    std::cout << text->text << "\n";
                }
            }
        }
    }

    // Show tool results count
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

    // Show token usage
    if (result.total_usage.total_input_tokens > 0 || result.total_usage.total_output_tokens > 0) {
        std::cout << "["
                  << result.total_usage.total_input_tokens << " input, "
                  << result.total_usage.total_output_tokens << " output tokens]\n";
    }

    std::cout << "\n";
}

}  // namespace ccmake
