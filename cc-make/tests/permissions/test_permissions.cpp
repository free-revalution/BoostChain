#include <catch2/catch_test_macros.hpp>
#include "permissions/permissions.hpp"
using namespace ccmake;

static PermissionCheckRule make_rule(const std::string& source, PermissionBehavior behavior,
                                      const std::string& tool_name, const std::string& content = "") {
    PermissionCheckRule r;
    r.source = source;
    r.behavior = behavior;
    r.tool_name = tool_name;
    r.rule_content = content;
    return r;
}

TEST_CASE("Default mode asks for unknown tool") {
    PermissionContext ctx;
    auto result = check_permission("Bash", "", ctx);
    REQUIRE(result.behavior == PermissionBehavior::Ask);
}

TEST_CASE("Bypass mode allows all tools") {
    PermissionContext ctx;
    ctx.mode = PermissionMode::BypassPermissions;
    auto result = check_permission("Bash", "", ctx);
    REQUIRE(result.behavior == PermissionBehavior::Allow);
}

TEST_CASE("Deny rule blocks tool") {
    PermissionContext ctx;
    auto r = make_rule("userSettings", PermissionBehavior::Deny, "Bash");
    add_rule(ctx, std::move(r));
    auto result = check_permission("Bash", "", ctx);
    REQUIRE(result.behavior == PermissionBehavior::Deny);
    REQUIRE(result.decision_reason == "rule");
}

TEST_CASE("Allow rule permits tool") {
    PermissionContext ctx;
    auto r = make_rule("userSettings", PermissionBehavior::Allow, "Read");
    add_rule(ctx, std::move(r));
    auto result = check_permission("Read", "", ctx);
    REQUIRE(result.behavior == PermissionBehavior::Allow);
}

TEST_CASE("Deny takes priority over allow") {
    PermissionContext ctx;
    auto r1 = make_rule("userSettings", PermissionBehavior::Allow, "Bash");
    auto r2 = make_rule("projectSettings", PermissionBehavior::Deny, "Bash");
    add_rule(ctx, std::move(r1));
    add_rule(ctx, std::move(r2));
    auto result = check_permission("Bash", "", ctx);
    REQUIRE(result.behavior == PermissionBehavior::Deny);
}

TEST_CASE("Wildcard deny rule") {
    PermissionContext ctx;
    auto r = make_rule("userSettings", PermissionBehavior::Deny, "*");
    add_rule(ctx, std::move(r));
    auto result = check_permission("Anything", "", ctx);
    REQUIRE(result.behavior == PermissionBehavior::Deny);
}

TEST_CASE("is_denied checks deny rules") {
    PermissionContext ctx;
    auto r = make_rule("userSettings", PermissionBehavior::Deny, "Dangerous");
    add_rule(ctx, std::move(r));
    REQUIRE(is_denied("Dangerous", ctx));
    REQUIRE_FALSE(is_denied("Safe", ctx));
}

TEST_CASE("is_allowed checks allow rules") {
    PermissionContext ctx;
    auto r = make_rule("userSettings", PermissionBehavior::Allow, "Read");
    add_rule(ctx, std::move(r));
    REQUIRE(is_allowed("Read", ctx));
    REQUIRE_FALSE(is_allowed("Write", ctx));
}

TEST_CASE("Plan mode asks for tools") {
    PermissionContext ctx;
    ctx.mode = PermissionMode::Plan;
    auto result = check_permission("Write", "", ctx);
    REQUIRE(result.behavior == PermissionBehavior::Ask);
}

TEST_CASE("Multiple rules first match wins by type") {
    PermissionContext ctx;
    auto r1 = make_rule("a", PermissionBehavior::Allow, "Tool");
    auto r2 = make_rule("b", PermissionBehavior::Ask, "Tool");
    add_rule(ctx, std::move(r1));
    add_rule(ctx, std::move(r2));
    auto result = check_permission("Tool", "", ctx);
    REQUIRE(result.behavior == PermissionBehavior::Allow);
    REQUIRE(result.decision_reason == "rule");
}

TEST_CASE("No rules and bypass mode") {
    PermissionContext ctx;
    ctx.mode = PermissionMode::BypassPermissions;
    auto result = check_permission("Unknown", "", ctx);
    REQUIRE(result.behavior == PermissionBehavior::Allow);
    REQUIRE(result.decision_reason == "mode");
}
