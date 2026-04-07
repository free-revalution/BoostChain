#include <catch2/catch_test_macros.hpp>
#include "ui/markdown.hpp"

using namespace ccmake;

// ---- parse tests ----

TEST_CASE("MarkdownRenderer parse heading1") {
    MarkdownRenderer renderer;
    auto blocks = renderer.parse("# Title");
    REQUIRE(blocks.size() == 1);
    REQUIRE(blocks[0].type == MarkdownBlock::Type::Heading1);
    REQUIRE(blocks[0].content == "Title");
}

TEST_CASE("MarkdownRenderer parse heading2") {
    MarkdownRenderer renderer;
    auto blocks = renderer.parse("## Subtitle");
    REQUIRE(blocks.size() == 1);
    REQUIRE(blocks[0].type == MarkdownBlock::Type::Heading2);
    REQUIRE(blocks[0].content == "Subtitle");
}

TEST_CASE("MarkdownRenderer parse heading3") {
    MarkdownRenderer renderer;
    auto blocks = renderer.parse("### Section");
    REQUIRE(blocks.size() == 1);
    REQUIRE(blocks[0].type == MarkdownBlock::Type::Heading3);
    REQUIRE(blocks[0].content == "Section");
}

TEST_CASE("MarkdownRenderer parse code block") {
    MarkdownRenderer renderer;
    auto blocks = renderer.parse("```\ncode line\n```");
    REQUIRE(blocks.size() == 1);
    REQUIRE(blocks[0].type == MarkdownBlock::Type::CodeBlock);
    REQUIRE(blocks[0].content == "code line");
    REQUIRE(blocks[0].language.empty());
}

TEST_CASE("MarkdownRenderer parse code block with language") {
    MarkdownRenderer renderer;
    auto blocks = renderer.parse("```cpp\nint x = 42;\n```");
    REQUIRE(blocks.size() == 1);
    REQUIRE(blocks[0].type == MarkdownBlock::Type::CodeBlock);
    REQUIRE(blocks[0].content == "int x = 42;");
    REQUIRE(blocks[0].language == "cpp");
}

TEST_CASE("MarkdownRenderer parse bullet list") {
    MarkdownRenderer renderer;
    auto blocks = renderer.parse("- item");
    REQUIRE(blocks.size() == 1);
    REQUIRE(blocks[0].type == MarkdownBlock::Type::BulletList);
    REQUIRE(blocks[0].content == "item");
}

TEST_CASE("MarkdownRenderer parse numbered list") {
    MarkdownRenderer renderer;
    auto blocks = renderer.parse("1. item");
    REQUIRE(blocks.size() == 1);
    REQUIRE(blocks[0].type == MarkdownBlock::Type::NumberedList);
    REQUIRE(blocks[0].content == "item");
}

TEST_CASE("MarkdownRenderer parse blockquote") {
    MarkdownRenderer renderer;
    auto blocks = renderer.parse("> quote");
    REQUIRE(blocks.size() == 1);
    REQUIRE(blocks[0].type == MarkdownBlock::Type::Blockquote);
    REQUIRE(blocks[0].content == "quote");
}

TEST_CASE("MarkdownRenderer parse horizontal rule") {
    MarkdownRenderer renderer;
    auto blocks = renderer.parse("---");
    REQUIRE(blocks.size() == 1);
    REQUIRE(blocks[0].type == MarkdownBlock::Type::HorizontalRule);
}

TEST_CASE("MarkdownRenderer parse blank line") {
    MarkdownRenderer renderer;
    auto blocks = renderer.parse("");
    REQUIRE(blocks.size() == 1);
    REQUIRE(blocks[0].type == MarkdownBlock::Type::BlankLine);
    REQUIRE(blocks[0].content.empty());
}

TEST_CASE("MarkdownRenderer parse text") {
    MarkdownRenderer renderer;
    auto blocks = renderer.parse("Hello world");
    REQUIRE(blocks.size() == 1);
    REQUIRE(blocks[0].type == MarkdownBlock::Type::Text);
    REQUIRE(blocks[0].content == "Hello world");
}

// ---- render tests ----

TEST_CASE("MarkdownRenderer render heading includes bold") {
    MarkdownRenderer renderer;
    auto blocks = renderer.parse("# Title");
    std::string output = renderer.render(blocks);
    // ANSI bold is \x1b[1m
    REQUIRE(output.find("\x1b[1m") != std::string::npos);
    REQUIRE(output.find("Title") != std::string::npos);
}

TEST_CASE("MarkdownRenderer render code block preserves content") {
    MarkdownRenderer renderer;
    auto blocks = renderer.parse("```cpp\nint x = 42;\n```");
    std::string output = renderer.render(blocks);
    REQUIRE(output.find("int x = 42;") != std::string::npos);
    REQUIRE(output.find("cpp") != std::string::npos);
}

TEST_CASE("MarkdownRenderer render bullet list has prefix") {
    MarkdownRenderer renderer;
    auto blocks = renderer.parse("- item");
    std::string output = renderer.render(blocks);
    // Bullet character is UTF-8: \xe2\x80\xa2
    REQUIRE(output.find("\xe2\x80\xa2") != std::string::npos);
    REQUIRE(output.find("item") != std::string::npos);
}

TEST_CASE("MarkdownRenderer render inline bold") {
    MarkdownRenderer renderer;
    std::string output = renderer.render_string("This is **bold** text");
    REQUIRE(output.find("\x1b[1m") != std::string::npos);
    REQUIRE(output.find("bold") != std::string::npos);
}

TEST_CASE("MarkdownRenderer render inline italic") {
    MarkdownRenderer renderer;
    std::string output = renderer.render_string("This is *italic* text");
    // ANSI italic is \x1b[3m
    REQUIRE(output.find("\x1b[3m") != std::string::npos);
    REQUIRE(output.find("italic") != std::string::npos);
}

TEST_CASE("MarkdownRenderer render inline code") {
    MarkdownRenderer renderer;
    std::string output = renderer.render_string("Use `code` here");
    // ANSI reverse is \x1b[7m
    REQUIRE(output.find("\x1b[7m") != std::string::npos);
    REQUIRE(output.find("code") != std::string::npos);
}

TEST_CASE("MarkdownRenderer render inline link") {
    MarkdownRenderer renderer;
    std::string output = renderer.render_string("See [docs](https://example.com)");
    REQUIRE(output.find("docs") != std::string::npos);
    REQUIRE(output.find("https://example.com") != std::string::npos);
}

TEST_CASE("MarkdownRenderer render_string convenience") {
    MarkdownRenderer renderer;
    std::string output = renderer.render_string("# Hello\n\nWorld");
    REQUIRE(output.find("Hello") != std::string::npos);
    REQUIRE(output.find("World") != std::string::npos);
}

// ---- streaming tests ----

TEST_CASE("MarkdownRenderer feed streaming") {
    MarkdownRenderer renderer;
    // Feed partial lines -- nothing should be output yet
    auto result1 = renderer.feed("Hello");
    REQUIRE(result1.empty());

    // Feed newline to complete the line
    auto result2 = renderer.feed("\n");
    REQUIRE_FALSE(result2.empty());
    REQUIRE(result2[0].find("Hello") != std::string::npos);
}

TEST_CASE("MarkdownRenderer feed reset") {
    MarkdownRenderer renderer;
    renderer.feed("partial");
    REQUIRE(renderer.use_colors() == true);  // default is true
    renderer.reset();
    // After reset, the buffer is cleared. Feed a newline -- since buffer
    // is empty, this produces an empty line which parses to a BlankLine.
    // But importantly, "partial" should NOT appear in output.
    auto result = renderer.feed("\n");
    for (const auto& line : result) {
        REQUIRE(line.find("partial") == std::string::npos);
    }
}
