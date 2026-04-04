#pragma once

#include <string>
#include <vector>

namespace ccmake {

enum class CLIMode {
    Interactive,
    Pipe,
    Print,
    Version,
    Help,
    Resume,
    Doctor
};

struct CLIArgs {
    CLIMode mode = CLIMode::Interactive;
    std::string prompt;
    std::string model;
    std::string session_id;
    std::string output_format;
    bool verbose = false;
    bool debug = false;
    std::vector<std::string> files;
};

int run_cli(int argc, char** argv);

}  // namespace ccmake
