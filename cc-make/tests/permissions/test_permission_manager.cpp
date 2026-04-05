#include <catch2/catch_test_macros.hpp>
#include "permissions/permission_manager.hpp"
using namespace ccmake;

// ============================================================
// PermissionManager unit tests
// ============================================================

TEST_CASE("PermissionManager default mode asks for tools") {
    PermissionManager pm;
    auto result = pm.check("Bash");
    REQUIRE(result.behavior == PermissionBehavior::Ask);
}

TEST_CASE("PermissionManager bypass mode allows all") {
    PermissionManager pm;
    pm.set_mode(PermissionMode::BypassPermissions);
    auto result = pm.check("Bash");
    REQUIRE(result.behavior == PermissionBehavior::Allow);
    REQUIRE(result.decision_reason == "mode");
}

TEST_CASE("PermissionManager add allow rule") {
    PermissionManager pm;
    pm.add_allow_rule("Read");
    auto result = pm.check("Read");
    REQUIRE(result.behavior == PermissionBehavior::Allow);
}

TEST_CASE("PermissionManager add deny rule") {
    PermissionManager pm;
    pm.add_deny_rule("Bash");
    auto result = pm.check("Bash");
    REQUIRE(result.behavior == PermissionBehavior::Deny);
}

TEST_CASE("PermissionManager deny takes priority") {
    PermissionManager pm;
    pm.add_allow_rule("Bash");
    pm.add_deny_rule("Bash");
    auto result = pm.check("Bash");
    REQUIRE(result.behavior == PermissionBehavior::Deny);
}

TEST_CASE("PermissionManager wildcard rules") {
    PermissionManager pm;
    pm.add_allow_rule("*");
    auto result = pm.check("Anything");
    REQUIRE(result.behavior == PermissionBehavior::Allow);
}

TEST_CASE("PermissionManager clear rules") {
    PermissionManager pm;
    pm.add_allow_rule("Bash");
    pm.clear_rules();
    auto result = pm.check("Bash");
    REQUIRE(result.behavior == PermissionBehavior::Ask);
}

TEST_CASE("PermissionManager set mode") {
    PermissionManager pm;
    REQUIRE(pm.mode() == PermissionMode::Default);
    pm.set_mode(PermissionMode::AcceptEdits);
    REQUIRE(pm.mode() == PermissionMode::AcceptEdits);
    auto result = pm.check("Write");
    REQUIRE(result.behavior == PermissionBehavior::Allow);
}

// ============================================================
// Memoization tests
// ============================================================

TEST_CASE("PermissionManager memoization: approved tools auto-allowed") {
    PermissionManager pm;
    // First check returns Ask (no callback)
    auto r1 = pm.check("Bash");
    REQUIRE(r1.behavior == PermissionBehavior::Ask);

    // Simulate approval via check_with_memo with callback
    pm.set_approval_callback([](const std::string&, const std::string&) { return true; });
    auto r2 = pm.check_with_memo("Bash");
    REQUIRE(r2.behavior == PermissionBehavior::Allow);

    // Now it should be memoized — no callback needed
    pm.set_approval_callback(nullptr);
    auto r3 = pm.check_with_memo("Bash");
    REQUIRE(r3.behavior == PermissionBehavior::Allow);
    REQUIRE(r3.decision_reason == "memo");
}

TEST_CASE("PermissionManager memoization: denied tools blocked") {
    PermissionManager pm;
    pm.set_approval_callback([](const std::string&, const std::string&) { return false; });
    auto result = pm.check_with_memo("Bash");
    REQUIRE(result.behavior == PermissionBehavior::Deny);
    REQUIRE(result.decision_reason == "user_denied");
}

TEST_CASE("PermissionManager memoization: no callback returns Ask") {
    PermissionManager pm;
    auto result = pm.check_with_memo("Bash");
    REQUIRE(result.behavior == PermissionBehavior::Ask);
}

TEST_CASE("PermissionManager reset_approvals clears memo") {
    PermissionManager pm;
    pm.set_approval_callback([](const std::string&, const std::string&) { return true; });
    pm.check_with_memo("Bash");
    REQUIRE(pm.is_approved("Bash"));

    pm.reset_approvals();
    REQUIRE_FALSE(pm.is_approved("Bash"));
}

TEST_CASE("PermissionManager is_approved") {
    PermissionManager pm;
    REQUIRE_FALSE(pm.is_approved("Bash"));

    pm.set_approval_callback([](const std::string&, const std::string&) { return true; });
    pm.check_with_memo("Bash");
    REQUIRE(pm.is_approved("Bash"));
}

// ============================================================
// Rule source tracking
// ============================================================

TEST_CASE("PermissionManager rule source is preserved") {
    PermissionManager pm;
    pm.add_allow_rule("Read", "projectSettings");
    pm.add_deny_rule("Bash", "userSettings");

    auto r1 = pm.check("Read");
    REQUIRE(r1.behavior == PermissionBehavior::Allow);

    auto r2 = pm.check("Bash");
    REQUIRE(r2.behavior == PermissionBehavior::Deny);
    REQUIRE(r2.decision_reason == "rule");
    REQUIRE(r2.message.find("userSettings") != std::string::npos);
}

// ============================================================
// AcceptEdits mode
// ============================================================

TEST_CASE("PermissionManager AcceptEdits mode allows everything") {
    PermissionManager pm;
    pm.set_mode(PermissionMode::AcceptEdits);
    auto result = pm.check("Write");
    REQUIRE(result.behavior == PermissionBehavior::Allow);
}

// ============================================================
// Plan mode
// ============================================================

TEST_CASE("PermissionManager Plan mode asks for tools") {
    PermissionManager pm;
    pm.set_mode(PermissionMode::Plan);
    auto result = pm.check("Bash");
    REQUIRE(result.behavior == PermissionBehavior::Ask);
    REQUIRE(result.message.find("Plan mode") != std::string::npos);
}
