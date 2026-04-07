#include <catch2/catch_test_macros.hpp>
#include "tools/ask/ask_question_tool.hpp"
using namespace ccmake;

TEST_CASE("AskUserQuestionTool definition", "[ask]") {
    AskUserQuestionTool tool;
    auto& def = tool.definition();
    REQUIRE(def.name == "AskUserQuestion");
    REQUIRE_FALSE(def.description.empty());
}

TEST_CASE("AskUserQuestionTool is read only", "[ask]") {
    AskUserQuestionTool tool;
    auto& def = tool.definition();
    REQUIRE(def.is_read_only);
    REQUIRE_FALSE(def.is_destructive);
}

TEST_CASE("AskUserQuestionTool execute returns question", "[ask]") {
    AskUserQuestionTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"question", "What is your name?"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("What is your name?") != std::string::npos);
}

TEST_CASE("AskUserQuestionTool execute with options", "[ask]") {
    AskUserQuestionTool tool;
    ToolContext ctx;
    auto result = tool.execute({
        {"question", "Choose a language"},
        {"options", {
            {{"label", "C++"}, {"description", "A systems programming language"}},
            {{"label", "Python"}, {"description", "A scripting language"}}
        }}
    }, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("Choose a language") != std::string::npos);
    REQUIRE(result.content.find("C++") != std::string::npos);
    REQUIRE(result.content.find("Python") != std::string::npos);
    REQUIRE(result.content.find("systems programming") != std::string::npos);
}

TEST_CASE("AskUserQuestionTool validate missing question", "[ask]") {
    AskUserQuestionTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({}, ctx);
    REQUIRE_FALSE(err.empty());
}

TEST_CASE("AskUserQuestionTool validate empty question", "[ask]") {
    AskUserQuestionTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({{"question", ""}}, ctx);
    REQUIRE_FALSE(err.empty());
}

TEST_CASE("AskUserQuestionTool validate accepts valid input", "[ask]") {
    AskUserQuestionTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({{"question", "Hello?"}}, ctx);
    REQUIRE(err.empty());
}
