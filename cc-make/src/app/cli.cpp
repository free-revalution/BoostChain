#include "app/cli.hpp"
#include "app/app_state.hpp"
#include "app/repl.hpp"
#include "auth/auth.hpp"
#include "config/config_loader.hpp"
#include "query/query_engine.hpp"
#include "tools/file/read_tool.hpp"
#include "tools/file/edit_tool.hpp"
#include "tools/file/write_tool.hpp"
#include "tools/bash/bash_tool.hpp"
#include "tools/search/glob_tool.hpp"
#include "tools/search/grep_tool.hpp"

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include <iostream>
#include <filesystem>
#include <unistd.h>

namespace ccmake {

namespace {

const char* version_string() {
    return "cc-make v0.1.0";
}

}  // anonymous namespace

int run_cli(int argc, char** argv) {
    CLI::App app{"CC-make: C++ Claude Code"};

    // CLI arguments
    CLIArgs args;

    // Positional prompt
    app.add_option("prompt", args.prompt, "Initial prompt");

    // Version flag
    app.add_flag("-v,--version", [&](bool) {
        args.mode = CLIMode::Version;
    }, "Print version and exit");

    // Model option
    app.add_option("-m,--model", args.model, "Model to use");

    // Resume session
    app.add_option("--resume", args.session_id, "Resume a previous session");

    // Output format
    app.add_option("--output-format", args.output_format, "Output format (text, json, stream-json)")
        ->check(CLI::IsMember({"text", "json", "stream-json"}));

    // Verbose logging
    app.add_flag("--verbose", args.verbose, "Enable verbose logging");

    // Debug logging
    app.add_flag("--debug", args.debug, "Enable debug logging");

    // Print system prompt
    app.add_flag("--print-system-prompt", [&](bool) {
        args.mode = CLIMode::Print;
    }, "Print the system prompt and exit");

    // Doctor mode
    app.add_flag("--doctor", [&](bool) {
        args.mode = CLIMode::Doctor;
    }, "Run diagnostic checks");

    // Attachment option (repeatable)
    app.add_option("-a,--attachment", args.files, "Attach files to the prompt");

    // CLI11 handles --help automatically, so we detect it via callback
    app.set_help_flag("", "");

    // Add a custom --help/-h flag
    bool help_requested = false;
    app.add_flag("-h,--help", help_requested, "Print this help message and exit");

    // Parse arguments
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    // Handle help request manually
    if (help_requested) {
        args.mode = CLIMode::Help;
        std::cout << app.help() << std::endl;
        return 0;
    }

    // Configure spdlog log level
    if (args.debug) {
        spdlog::set_level(spdlog::level::debug);
    } else if (args.verbose) {
        spdlog::set_level(spdlog::level::debug);
    } else {
        spdlog::set_level(spdlog::level::info);
    }

    // Handle version mode
    if (args.mode == CLIMode::Version) {
        spdlog::info(version_string());
        return 0;
    }

    // Determine CLIMode based on flags and environment
    if (args.mode == CLIMode::Interactive) {
        if (!args.session_id.empty()) {
            args.mode = CLIMode::Resume;
        } else if (!isatty(STDIN_FILENO) || !args.prompt.empty()) {
            args.mode = CLIMode::Pipe;
        }
    }

    // Resolve model from args or environment
    if (args.model.empty()) {
        auto env_model = get_model_from_env();
        if (env_model) {
            args.model = *env_model;
        }
    }

    // Log mode and model info
    std::string mode_str;
    switch (args.mode) {
        case CLIMode::Interactive: mode_str = "Interactive"; break;
        case CLIMode::Pipe:        mode_str = "Pipe"; break;
        case CLIMode::Print:       mode_str = "Print"; break;
        case CLIMode::Version:     mode_str = "Version"; break;
        case CLIMode::Help:        mode_str = "Help"; break;
        case CLIMode::Resume:      mode_str = "Resume"; break;
        case CLIMode::Doctor:      mode_str = "Doctor"; break;
    }
    spdlog::debug("CLI mode: {}", mode_str);

    if (!args.model.empty()) {
        spdlog::debug("Model: {}", args.model);
    }

    if (args.mode == CLIMode::Pipe && !args.prompt.empty()) {
        spdlog::debug("Prompt: {}", args.prompt);
    }

    // Print/Doctor modes are stubs for now — they will be implemented in later phases
    if (args.mode == CLIMode::Print) {
        spdlog::info("System prompt printing not yet implemented");
        return 0;
    }

    if (args.mode == CLIMode::Doctor) {
        spdlog::info("Doctor mode not yet implemented");
        return 0;
    }

    // REPL/interactive loop
    // Build query engine with default tools
    std::string cwd = std::filesystem::current_path().string();

    QueryEngine engine(args.model.empty() ? "claude-sonnet-4-20250514" : args.model);
    engine.set_cwd(cwd);

    // Load and apply config
    auto settings = build_runtime_settings(cwd);
    if (!settings.effective_model.empty() && args.model.empty()) {
        engine.set_model(settings.effective_model);
    }
    apply_config_to_engine(settings.global_config, engine);

    // Register built-in tools
    engine.register_tool(std::make_unique<ReadTool>());
    engine.register_tool(std::make_unique<EditTool>());
    engine.register_tool(std::make_unique<WriteTool>());
    engine.register_tool(std::make_unique<BashTool>());
    engine.register_tool(std::make_unique<GlobTool>());
    engine.register_tool(std::make_unique<GrepTool>());

    // Create REPL
    Repl repl(engine, args);

    if (args.mode == CLIMode::Pipe && !args.prompt.empty()) {
        return repl.process_prompt(args.prompt);
    }

    if (args.mode == CLIMode::Interactive) {
        return repl.run();
    }

    return 0;
}

}  // namespace ccmake
