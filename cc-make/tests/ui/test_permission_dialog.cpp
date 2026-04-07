#include <catch2/catch_test_macros.hpp>
#include "ui/permission_dialog.hpp"

using namespace ccmake;

// ---------------------------------------------------------------------------
// Render tests
// ---------------------------------------------------------------------------

TEST_CASE("PermissionDialog render basic prompt", "[permission_dialog]") {
    PermissionDialog dialog;
    dialog.set_use_colors(false);

    PermissionPrompt prompt;
    prompt.tool_name = "Bash";
    prompt.input_summary = "ls -la";
    prompt.full_input = "ls -la";
    prompt.is_destructive = false;
    prompt.is_new_tool = false;

    std::string result = dialog.render(prompt);

    REQUIRE(result.find("Tool: Bash") != std::string::npos);
    REQUIRE(result.find("Input: ls -la") != std::string::npos);
}

TEST_CASE("PermissionDialog render destructive prompt", "[permission_dialog]") {
    PermissionDialog dialog;
    dialog.set_use_colors(false);

    PermissionPrompt prompt;
    prompt.tool_name = "Bash";
    prompt.input_summary = "rm -rf /";
    prompt.full_input = "rm -rf /";
    prompt.is_destructive = true;
    prompt.is_new_tool = false;

    std::string result = dialog.render(prompt);

    REQUIRE(result.find("Destructive action") != std::string::npos);
    REQUIRE(result.find("Action: Destructive") != std::string::npos);
}

TEST_CASE("PermissionDialog render new tool prompt", "[permission_dialog]") {
    PermissionDialog dialog;
    dialog.set_use_colors(false);

    PermissionPrompt prompt;
    prompt.tool_name = "Write";
    prompt.input_summary = "/tmp/test.txt";
    prompt.full_input = "/tmp/test.txt";
    prompt.is_destructive = false;
    prompt.is_new_tool = true;

    std::string result = dialog.render(prompt);

    REQUIRE(result.find("(new tool)") != std::string::npos);
}

TEST_CASE("PermissionDialog render truncates long input", "[permission_dialog]") {
    PermissionDialog dialog;
    dialog.set_use_colors(false);

    PermissionPrompt prompt;
    prompt.tool_name = "Bash";
    prompt.input_summary = "";
    prompt.full_input = std::string(300, 'x');  // 300 chars
    prompt.is_destructive = false;
    prompt.is_new_tool = false;

    std::string result = dialog.render(prompt);

    // The input line should be truncated (first 200 chars + "...")
    // Total display length for the input value part should be 200
    // (197 chars + "...")
    REQUIRE(result.find("...") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Parse response tests
// ---------------------------------------------------------------------------

TEST_CASE("PermissionDialog parse yes", "[permission_dialog]") {
    PermissionDialog dialog;
    REQUIRE(dialog.parse_response("yes") == PromptResult::Allow);
}

TEST_CASE("PermissionDialog parse y", "[permission_dialog]") {
    PermissionDialog dialog;
    REQUIRE(dialog.parse_response("y") == PromptResult::Allow);
}

TEST_CASE("PermissionDialog parse no", "[permission_dialog]") {
    PermissionDialog dialog;
    REQUIRE(dialog.parse_response("no") == PromptResult::Deny);
}

TEST_CASE("PermissionDialog parse always", "[permission_dialog]") {
    PermissionDialog dialog;
    REQUIRE(dialog.parse_response("always") == PromptResult::AllowAlways);
}

TEST_CASE("PermissionDialog parse escape", "[permission_dialog]") {
    PermissionDialog dialog;
    REQUIRE(dialog.parse_response("escape") == PromptResult::Abort);
}

TEST_CASE("PermissionDialog parse unknown defaults to deny", "[permission_dialog]") {
    PermissionDialog dialog;
    REQUIRE(dialog.parse_response("foo") == PromptResult::Deny);
}

// ---------------------------------------------------------------------------
// Format result tests
// ---------------------------------------------------------------------------

TEST_CASE("PermissionDialog format_result allow", "[permission_dialog]") {
    std::string result = PermissionDialog::format_result(PromptResult::Allow, "Bash");
    REQUIRE(result.find("Allowed") != std::string::npos);
    REQUIRE(result.find("Bash") != std::string::npos);
}

TEST_CASE("PermissionDialog format_result deny", "[permission_dialog]") {
    std::string result = PermissionDialog::format_result(PromptResult::Deny, "Write");
    REQUIRE(result.find("Denied") != std::string::npos);
    REQUIRE(result.find("Write") != std::string::npos);
}

TEST_CASE("PermissionDialog format_result abort", "[permission_dialog]") {
    std::string result = PermissionDialog::format_result(PromptResult::Abort, "");
    REQUIRE(result.find("Aborted") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Build summary tests
// ---------------------------------------------------------------------------

TEST_CASE("PermissionDialog build_summary for Bash", "[permission_dialog]") {
    PermissionDialog dialog;
    dialog.set_use_colors(false);

    PermissionPrompt prompt;
    prompt.tool_name = "Bash";
    prompt.input_summary = "list files";
    prompt.full_input = "ls -la /home/user";
    prompt.is_destructive = false;

    // For Bash, build_summary should show the command from full_input
    std::string rendered = dialog.render(prompt);
    REQUIRE(rendered.find("ls -la /home/user") != std::string::npos);
}

TEST_CASE("PermissionDialog build_summary for Write", "[permission_dialog]") {
    PermissionDialog dialog;
    dialog.set_use_colors(false);

    PermissionPrompt prompt;
    prompt.tool_name = "Write";
    prompt.input_summary = "/tmp/test.txt";
    prompt.full_input = "/tmp/test.txt";
    prompt.is_destructive = false;

    // For Write, build_summary should show the file path
    std::string rendered = dialog.render(prompt);
    REQUIRE(rendered.find("/tmp/test.txt") != std::string::npos);
}
