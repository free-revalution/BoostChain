#pragma once

#include "app/cli.hpp"
#include "app/app_state.hpp"
#include "app/commands.hpp"
#include "query/query_engine.hpp"
#include "session/session.hpp"
#include "terminal/theme.hpp"
#include "terminal/keybinding.hpp"
#include "ui/streaming_renderer.hpp"

#include <string>
#include <functional>
#include <optional>
#include <iomanip>

namespace ccmake {

// Forward declaration
class McpManager;

class Repl {
public:
    Repl(QueryEngine& engine, const CLIArgs& args);

    // Run the REPL loop. Returns exit code.
    int run();

    // Process a single prompt (non-interactive). Returns exit code.
    int process_prompt(const std::string& prompt);

    // Handle a slash command. Returns true if handled.
    bool handle_command(const std::string& input);

    // Load a session into the engine
    bool resume_session(const std::string& session_id);

    // Create a new session and enable auto-save
    void init_session();

    // Set MCP manager from CLI (called after construction if MCP is available)
    void set_mcp_manager(void* mgr);

    // Accessors for commands
    QueryEngine& engine() { return engine_; }
    CommandRegistry& command_registry() { return command_registry_; }
    ThemeManager& theme_manager() { return theme_manager_; }
    KeybindingManager& keybinding_manager() { return keybinding_manager_; }
    void set_running(bool running) { running_ = running; }
    bool is_running() const { return running_; }
    SessionStore& session_store() { return session_store_; }
    bool vim_mode() const { return vim_mode_; }
    void set_vim_mode(bool enabled) { vim_mode_ = enabled; }

private:
    void print_welcome();
    void print_help();
    std::string read_line();
    void display_response(const TurnResult& result);

    QueryEngine& engine_;
    CLIArgs args_;
    SessionStore session_store_;
    StreamingRenderer renderer_;
    CommandRegistry command_registry_;
    ThemeManager theme_manager_;
    KeybindingManager keybinding_manager_;
    bool running_ = true;
    bool vim_mode_ = false;
};

}  // namespace ccmake
