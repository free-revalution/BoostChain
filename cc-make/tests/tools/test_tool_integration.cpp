#include <catch2/catch_test_macros.hpp>
#include "query/query_engine.hpp"
#include "tools/file/read_tool.hpp"
#include "tools/file/edit_tool.hpp"
#include "tools/file/write_tool.hpp"
#include "tools/bash/bash_tool.hpp"
#include "tools/search/glob_tool.hpp"
#include "tools/search/grep_tool.hpp"
#include "tools/tool.hpp"
#include "tools/registry.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
using namespace ccmake;

// ============================================================
// ToolRegistry tests
// ============================================================

TEST_CASE("ToolRegistry register and find") {
    ToolRegistry reg;
    auto tool = std::make_unique<ReadTool>();
    REQUIRE(reg.register_tool(std::move(tool)));
    REQUIRE(reg.has("Read"));
    REQUIRE(reg.has("FileRead"));  // alias
    REQUIRE(reg.find("Read") != nullptr);
}

TEST_CASE("ToolRegistry duplicate registration returns false") {
    ToolRegistry reg;
    reg.register_tool(std::make_unique<ReadTool>());
    REQUIRE_FALSE(reg.register_tool(std::make_unique<ReadTool>()));
}

TEST_CASE("ToolRegistry all_definitions") {
    ToolRegistry reg;
    reg.register_tool(std::make_unique<ReadTool>());
    reg.register_tool(std::make_unique<EditTool>());
    auto defs = reg.all_definitions();
    REQUIRE(defs.size() == 2);
}

TEST_CASE("ToolRegistry remove") {
    ToolRegistry reg;
    reg.register_tool(std::make_unique<ReadTool>());
    reg.remove("Read");
    REQUIRE_FALSE(reg.has("Read"));
}

// ============================================================
// QueryEngine tool integration
// ============================================================

TEST_CASE("QueryEngine register ToolBase and execute via agentic loop") {
    static int counter = 0;
    auto test_file = "/tmp/ccmake_integration_" + std::to_string(counter++) + ".txt";
    { std::ofstream f(test_file); f << "original content\n"; }

    QueryEngine engine("test-model");
    engine.set_cwd("/tmp");
    engine.set_permission_mode(PermissionMode::BypassPermissions);
    engine.register_tool(std::make_unique<ReadTool>());
    engine.register_tool(std::make_unique<EditTool>());

    // Mock: model reads file, then edits it
    engine.set_mock_responses({
        Message::assistant("a1", {ToolUseBlock{"t1", "Read", {{"file_path", test_file}}}}),
        Message::assistant("a2", {ToolUseBlock{"t2", "Edit", {{"file_path", test_file}, {"old_string", "original"}, {"new_string", "modified"}}}}),
        Message::assistant("a3", {TextBlock{"Done editing."}})
    });

    auto result = engine.submit_message("Edit the file");
    REQUIRE(result.exit_reason == LoopExitReason::Completed);

    // Verify file was actually modified
    std::ifstream f(test_file);
    std::string content((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
    REQUIRE(content.find("modified") != std::string::npos);
    REQUIRE(content.find("original") == std::string::npos);

    std::filesystem::remove(test_file);
}

TEST_CASE("QueryEngine has_tool checks both registries") {
    QueryEngine engine("test-model");
    engine.register_tool("func_tool", [](const std::string&, const nlohmann::json&) {
        return "ok"_json;
    });
    REQUIRE(engine.has_tool("func_tool"));

    engine.register_tool(std::make_unique<ReadTool>());
    REQUIRE(engine.has_tool("Read"));
}

TEST_CASE("QueryEngine tool_registry provides definitions for API") {
    QueryEngine engine("test-model");
    engine.register_tool(std::make_unique<ReadTool>());
    engine.register_tool(std::make_unique<EditTool>());
    engine.register_tool(std::make_unique<WriteTool>());

    auto defs = engine.tool_registry().all_definitions();
    REQUIRE(defs.size() == 3);

    // Verify schemas are present
    for (const auto& def : defs) {
        REQUIRE_FALSE(def.name.empty());
        REQUIRE_FALSE(def.input_schema.is_null());
    }
}

// ============================================================
// End-to-end: Write + Read + Edit workflow
// ============================================================

TEST_CASE("Integration: Write then Read then Edit") {
    static int counter = 0;
    auto test_file = "/tmp/ccmake_e2e_" + std::to_string(counter++) + ".txt";

    ToolRegistry reg;
    reg.register_tool(std::make_unique<WriteTool>());
    reg.register_tool(std::make_unique<ReadTool>());
    reg.register_tool(std::make_unique<EditTool>());

    ToolContext ctx;

    // Step 1: Write
    auto* write_tool = reg.find("Write");
    REQUIRE(write_tool != nullptr);
    auto w_result = write_tool->execute({{"file_path", test_file}, {"content", "Hello World\n"}}, ctx);
    REQUIRE_FALSE(w_result.is_error);

    // Step 2: Read
    auto* read_tool = reg.find("Read");
    REQUIRE(read_tool != nullptr);
    auto r_result = read_tool->execute({{"file_path", test_file}}, ctx);
    REQUIRE_FALSE(r_result.is_error);
    REQUIRE(r_result.content.find("Hello World") != std::string::npos);

    // Step 3: Edit
    auto* edit_tool = reg.find("Edit");
    REQUIRE(edit_tool != nullptr);
    auto e_result = edit_tool->execute({{"file_path", test_file}, {"old_string", "Hello"}, {"new_string", "Goodbye"}}, ctx);
    REQUIRE_FALSE(e_result.is_error);

    // Verify
    std::ifstream f(test_file);
    std::string content((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
    REQUIRE(content == "Goodbye World\n");

    std::filesystem::remove(test_file);
}

// ============================================================
// Phase 6 tool integration: Bash, Glob, Grep
// ============================================================

TEST_CASE("QueryEngine register Bash + Glob + Grep tools") {
    QueryEngine engine("test-model");
    engine.register_tool(std::make_unique<BashTool>());
    engine.register_tool(std::make_unique<GlobTool>());
    engine.register_tool(std::make_unique<GrepTool>());

    REQUIRE(engine.has_tool("Bash"));
    REQUIRE(engine.has_tool("Glob"));
    REQUIRE(engine.has_tool("Grep"));

    auto defs = engine.tool_registry().all_definitions();
    REQUIRE(defs.size() == 3);
}

TEST_CASE("Integration: Bash echo + Read verify") {
    static int counter = 0;
    auto test_file = "/tmp/ccmake_bash_int_" + std::to_string(counter++) + ".txt";

    QueryEngine engine("test-model");
    engine.set_cwd("/tmp");
    engine.set_permission_mode(PermissionMode::BypassPermissions);
    engine.register_tool(std::make_unique<BashTool>());
    engine.register_tool(std::make_unique<ReadTool>());

    // Mock: model writes a file via bash, then reads it
    engine.set_mock_responses({
        Message::assistant("a1", {ToolUseBlock{"t1", "Bash", {{"command", "echo 'bash-created-content' > " + test_file}}}}),
        Message::assistant("a2", {ToolUseBlock{"t2", "Read", {{"file_path", test_file}}}}),
        Message::assistant("a3", {TextBlock{"Verified content."}})
    });

    auto result = engine.submit_message("Create and verify file");
    REQUIRE(result.exit_reason == LoopExitReason::Completed);
    REQUIRE(result.error_message.empty());

    std::filesystem::remove(test_file);
}

TEST_CASE("Integration: Glob + Grep workflow") {
    static int counter = 0;
    auto dir = "/tmp/ccmake_globgrep_" + std::to_string(counter++);
    std::filesystem::create_directories(dir);
    std::ofstream(dir + "/app.js") << "const x = 1;\n";
    std::ofstream(dir + "/util.js") << "const y = 2;\n";
    std::ofstream(dir + "/README.md") << "# Test\n";
    std::filesystem::create_directories(dir + "/src");
    std::ofstream(dir + "/src/main.cpp") << "int main() { return 0; }\n";

    QueryEngine engine("test-model");
    engine.set_cwd(dir);
    engine.set_permission_mode(PermissionMode::BypassPermissions);
    engine.register_tool(std::make_unique<GlobTool>());
    engine.register_tool(std::make_unique<GrepTool>());

    // Mock: model globs *.js then greps for a pattern
    engine.set_mock_responses({
        Message::assistant("a1", {ToolUseBlock{"t1", "Glob", {{"pattern", "*.js"}}}}),
        Message::assistant("a2", {ToolUseBlock{"t2", "Grep", {{"pattern", "const"}}}}),
        Message::assistant("a3", {TextBlock{"Found matches."}})
    });

    auto result = engine.submit_message("Find JS files and search for const");
    REQUIRE(result.exit_reason == LoopExitReason::Completed);
    REQUIRE(result.error_message.empty());

    // Verify the engine processed both tools (message history has tool results)
    auto msgs = engine.messages();
    int tool_results = 0;
    for (const auto& msg : msgs) {
        for (const auto& block : msg.content) {
            if (std::holds_alternative<ToolResultBlock>(block)) {
                ++tool_results;
            }
        }
    }
    REQUIRE(tool_results == 2);

    std::filesystem::remove_all(dir);
}

TEST_CASE("Integration: all Phase 5+6 tools registered together") {
    QueryEngine engine("test-model");
    engine.register_tool(std::make_unique<ReadTool>());
    engine.register_tool(std::make_unique<EditTool>());
    engine.register_tool(std::make_unique<WriteTool>());
    engine.register_tool(std::make_unique<BashTool>());
    engine.register_tool(std::make_unique<GlobTool>());
    engine.register_tool(std::make_unique<GrepTool>());

    REQUIRE(engine.has_tool("Read"));
    REQUIRE(engine.has_tool("Edit"));
    REQUIRE(engine.has_tool("Write"));
    REQUIRE(engine.has_tool("Bash"));
    REQUIRE(engine.has_tool("Glob"));
    REQUIRE(engine.has_tool("Grep"));

    auto defs = engine.tool_registry().all_definitions();
    REQUIRE(defs.size() == 6);

    // Verify all have schemas
    for (const auto& def : defs) {
        REQUIRE_FALSE(def.name.empty());
        REQUIRE_FALSE(def.input_schema.is_null());
    }
}
