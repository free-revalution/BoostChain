#include <boostchain/error.hpp>
#include <cassert>

void test_error_hierarchy() {
    try {
        throw boostchain::NetworkError("Connection failed", 500);
    } catch (const boostchain::Error& e) {
        assert(std::string(e.what()) == "Connection failed");
    }
}

void test_network_error_status_code() {
    boostchain::NetworkError err("Timeout", 408);
    assert(err.status_code() == 408);
}

void test_tool_error_name() {
    boostchain::ToolError err("calculator", "Division by zero");
    assert(err.tool_name() == "calculator");
    assert(std::string(err.what()).find("calculator") != std::string::npos);
}

int main() {
    test_error_hierarchy();
    test_network_error_status_code();
    test_tool_error_name();
    return 0;
}
