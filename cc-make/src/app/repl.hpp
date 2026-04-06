#pragma once

#include "app/cli.hpp"
#include "app/app_state.hpp"
#include "app/commands.hpp"
#include "query/query_engine.hpp"
#include "session/session.hpp"
#include "terminal/theme.hpp"
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

private:
    void print_welcome();
    void print_help();
    std::string read_line();
    void display_response(const TurnResult& result);
    void cmd_sessions();
    void cmd_resume(const std::string& arg);
    void cmd_new();

    QueryEngine& engine_;
    CLIArgs args_;
    SessionStore session_store_;
    StreamingRenderer renderer_;
    CommandRegistry command_registry_;
    ThemeManager theme_manager_;
    bool running_ = true;

    // TODO: McpManager* mcp_manager_ = nullptr;  // Set from CLI if available
    // TODO: KeybindingManager keybinding_manager_;
    // TODO: VimModeHandler vim_mode_;
};

}  // namespace ccmake
