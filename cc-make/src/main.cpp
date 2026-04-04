#include "app/cli.hpp"
#include <spdlog/spdlog.h>

int main(int argc, char** argv) {
    try {
        return ccmake::run_cli(argc, argv);
    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    } catch (...) {
        spdlog::error("Unknown fatal error");
        return 1;
    }
}
