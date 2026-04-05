#pragma once

#include "app/cli.hpp"
#include "app/app_state.hpp"
#include "query/query_engine.hpp"
#include "session/session.hpp"

#include <string>
#include <functional>
#include <optional>

namespace ccmake {

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
    bool running_ = true;
};

}  // namespace ccmake
