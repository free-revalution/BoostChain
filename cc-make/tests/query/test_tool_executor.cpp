#include <catch2/catch_test_macros.hpp>
#include "query/tool_executor.hpp"
#include "permissions/permission_manager.hpp"
#include <nlohmann/json.hpp>
using namespace ccmake;

TEST_CASE("ToolExecutor with echo tool") {
    ToolExecutor executor;
    executor.register_tool("echo", [](const std::string&, const nlohmann::json& input) {
        return input;
    });
    REQUIRE(executor.has_tool("echo"));
    REQUIRE_FALSE(executor.has_tool("nonexistent"));

    auto results = executor.execute({{"id", "echo", {{"text", "hello"}}}});
    REQUIRE(results.size() == 1);
    REQUIRE(results[0].tool_use_id == "id");
    REQUIRE(results[0].tool_name == "echo");
    REQUIRE(results[0].content == "{\"text\":\"hello\"}");
    REQUIRE_FALSE(results[0].is_error);
}

TEST_CASE("ToolExecutor with error tool") {
    ToolExecutor executor;
    executor.register_tool("fail", [](const std::string&, const nlohmann::json&) -> nlohmann::json {
        throw std::runtime_error("tool failed");
    });

    auto results = executor.execute({{"id2", "fail", {}}});
    REQUIRE(results.size() == 1);
    REQUIRE(results[0].tool_use_id == "id2");
    REQUIRE(results[0].is_error);
    REQUIRE(results[0].content.find("tool failed") != std::string::npos);
}

TEST_CASE("ToolExecutor with multiple tools") {
    ToolExecutor executor;
    executor.register_tool("a", [](const std::string&, const nlohmann::json& j) { return j; });
    executor.register_tool("b", [](const std::string&, const nlohmann::json& j) { return j; });

    auto results = executor.execute({
        {"id1", "a", {{"x", 1}}},
        {"id2", "b", {{"y", 2}}}
    });
    REQUIRE(results.size() == 2);
    REQUIRE(results[0].tool_use_id == "id1");
    REQUIRE(results[0].tool_name == "a");
    REQUIRE(results[1].tool_use_id == "id2");
    REQUIRE(results[1].tool_name == "b");
}

TEST_CASE("ToolExecutor empty input returns empty results") {
    ToolExecutor executor;
    auto results = executor.execute({});
    REQUIRE(results.empty());
}

TEST_CASE("ToolExecutor result struct fields") {
    ToolExecutor::ToolResult result;
    result.tool_use_id = "t1";
    result.tool_name = "test";
    result.content = "ok";
    result.is_error = false;
    REQUIRE(result.tool_use_id == "t1");
    result.tool_name == "test";
    REQUIRE(result.content == "ok");
    REQUIRE_FALSE(result.is_error);
}

TEST_CASE("ToolExecutor captures tool name in error") {
    ToolExecutor executor;
    executor.register_tool("bad_tool", [](const std::string& name, const nlohmann::json&) -> nlohmann::json {
        throw std::runtime_error(name + " failed");
    });
    auto results = executor.execute({{"id", "bad_tool", {}}});
    REQUIRE(results[0].content.find("bad_tool") != std::string::npos);
}

TEST_CASE("ToolExecutor unknown tool returns error") {
    ToolExecutor executor;
    auto results = executor.execute({{"id", "unknown_tool", {}}});
    REQUIRE(results.size() == 1);
    REQUIRE(results[0].is_error);
    REQUIRE(results[0].content.find("unknown tool") != std::string::npos);
}

TEST_CASE("ToolExecutor register_tool returns false for duplicate") {
    ToolExecutor executor;
    REQUIRE(executor.register_tool("dup", [](const std::string&, const nlohmann::json&) { return "ok"_json; }));
    REQUIRE_FALSE(executor.register_tool("dup", [](const std::string&, const nlohmann::json&) { return "ok"_json; }));
}

TEST_CASE("ToolExecutor clear removes all tools") {
    ToolExecutor executor;
    executor.register_tool("x", [](const std::string&, const nlohmann::json&) { return "ok"_json; });
    REQUIRE(executor.has_tool("x"));
    executor.clear();
    REQUIRE_FALSE(executor.has_tool("x"));
}

// ============================================================
// Permission integration in ToolExecutor
// ============================================================

TEST_CASE("ToolExecutor bypasses without permission manager") {
    ToolExecutor executor;
    executor.register_tool("echo", [](const std::string&, const nlohmann::json& j) { return j; });
    auto results = executor.execute({{"id", "echo", {{"text", "hi"}}}});
    REQUIRE_FALSE(results[0].is_error);
}

TEST_CASE("ToolExecutor denied tool blocked") {
    ToolExecutor executor;
    PermissionManager pm;
    pm.add_deny_rule("dangerous");
    executor.set_permission_manager(&pm);

    executor.register_tool("dangerous", [](const std::string&, const nlohmann::json&) {
        return nlohmann::json("should not run");
    });

    auto results = executor.execute({{"id", "dangerous", {}}});
    REQUIRE(results[0].is_error);
    REQUIRE(results[0].permission_denied);
    REQUIRE(results[0].content.find("Permission denied") != std::string::npos);
}

TEST_CASE("ToolExecutor allowed tool executes") {
    ToolExecutor executor;
    PermissionManager pm;
    pm.add_allow_rule("safe");
    executor.set_permission_manager(&pm);

    executor.register_tool("safe", [](const std::string&, const nlohmann::json&) {
        return nlohmann::json("executed");
    });

    auto results = executor.execute({{"id", "safe", {}}});
    REQUIRE_FALSE(results[0].is_error);
    REQUIRE_FALSE(results[0].permission_denied);
    REQUIRE(results[0].content.find("executed") != std::string::npos);
}

TEST_CASE("ToolExecutor ask without callback blocks") {
    ToolExecutor executor;
    PermissionManager pm;
    executor.set_permission_manager(&pm);

    executor.register_tool("needs_approval", [](const std::string&, const nlohmann::json&) {
        return nlohmann::json("should not run");
    });

    auto results = executor.execute({{"id", "needs_approval", {}}});
    REQUIRE(results[0].is_error);
    REQUIRE(results[0].permission_denied);
}

TEST_CASE("ToolExecutor ask with callback approved") {
    ToolExecutor executor;
    PermissionManager pm;
    pm.set_approval_callback([](const std::string&, const std::string&) { return true; });
    executor.set_permission_manager(&pm);

    executor.register_tool("needs_approval", [](const std::string&, const nlohmann::json&) {
        return nlohmann::json("approved");
    });

    auto results = executor.execute({{"id", "needs_approval", {}}});
    REQUIRE_FALSE(results[0].is_error);
    REQUIRE_FALSE(results[0].permission_denied);
}

TEST_CASE("ToolExecutor ask with callback denied") {
    ToolExecutor executor;
    PermissionManager pm;
    pm.set_approval_callback([](const std::string&, const std::string&) { return false; });
    executor.set_permission_manager(&pm);

    executor.register_tool("needs_approval", [](const std::string&, const nlohmann::json&) {
        return nlohmann::json("should not run");
    });

    auto results = executor.execute({{"id", "needs_approval", {}}});
    REQUIRE(results[0].is_error);
    REQUIRE(results[0].permission_denied);
    REQUIRE(results[0].content.find("User denied") != std::string::npos);
}

TEST_CASE("ToolExecutor memoized approval skips callback") {
    ToolExecutor executor;
    PermissionManager pm;
    int callback_count = 0;
    pm.set_approval_callback([&callback_count](const std::string&, const std::string&) {
        ++callback_count;
        return true;
    });
    executor.set_permission_manager(&pm);

    executor.register_tool("memo_test", [](const std::string&, const nlohmann::json&) {
        return nlohmann::json("ok");
    });

    // First call triggers callback
    auto r1 = executor.execute({{"id", "memo_test", {}}});
    REQUIRE_FALSE(r1[0].is_error);
    REQUIRE(callback_count == 1);

    // Second call should be memoized (no callback)
    auto r2 = executor.execute({{"id", "memo_test", {}}});
    REQUIRE_FALSE(r2[0].is_error);
    REQUIRE(callback_count == 1);  // not incremented
}

TEST_CASE("ToolExecutor bypass mode skips permission check") {
    ToolExecutor executor;
    PermissionManager pm;
    pm.set_mode(PermissionMode::BypassPermissions);
    executor.set_permission_manager(&pm);

    executor.register_tool("any_tool", [](const std::string&, const nlohmann::json&) {
        return nlohmann::json("bypassed");
    });

    auto results = executor.execute({{"id", "any_tool", {}}});
    REQUIRE_FALSE(results[0].is_error);
    REQUIRE_FALSE(results[0].permission_denied);
}
