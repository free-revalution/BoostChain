#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>
#include <string>

int main(int argc, char** argv) {
    CLI::App app{"CC-make: C++ Claude Code"};

    std::string prompt;
    app.add_option("prompt", prompt, "Initial prompt");

    bool version = false;
    app.add_flag("-v,--version", version, "Print version");

    CLI11_PARSE(app, argc, argv);

    if (version) {
        spdlog::info("cc-make v0.1.0");
        return 0;
    }

    spdlog::info("CC-make starting...");
    if (!prompt.empty()) {
        spdlog::info("Prompt: {}", prompt);
    }

    return 0;
}
