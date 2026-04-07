#pragma once
#include <string>
#include <vector>
#include <functional>

namespace ccmake {

// A rendered block of markdown
struct MarkdownBlock {
    enum class Type {
        Text,           // Normal paragraph text
        Heading1,       // # heading
        Heading2,       // ## heading
        Heading3,       // ### heading
        CodeBlock,      // ``` fenced code block ```
        InlineCode,     // `inline code`
        Bold,           // **bold text**
        Italic,         // *italic text*
        BulletList,     // - item
        NumberedList,   // 1. item
        Blockquote,     // > quote
        HorizontalRule, // ---
        Link,           // [text](url)
        BlankLine       // Empty line
    };

    Type type;
    std::string content;
    std::string language;  // For code blocks
};

// Markdown renderer that converts markdown to styled terminal output
class MarkdownRenderer {
public:
    MarkdownRenderer();

    // Parse markdown text into blocks
    std::vector<MarkdownBlock> parse(const std::string& markdown) const;

    // Render blocks to terminal output (with ANSI styling)
    std::string render(const std::vector<MarkdownBlock>& blocks) const;

    // Render a single markdown string to styled terminal output
    std::string render_string(const std::string& markdown) const;

    // Stream rendering: append partial markdown, get complete lines to output
    // Returns lines that are ready to be written to terminal
    std::vector<std::string> feed(const std::string& partial);

    // Reset streaming state
    void reset();

    // Set whether to use colors
    void set_use_colors(bool use);
    bool use_colors() const;

private:
    // Inline formatting
    std::string render_inline(const std::string& text) const;

    // Check if a line is a heading
    bool is_heading(const std::string& line, int& level) const;

    // Check if a line is a code fence
    bool is_code_fence(const std::string& line, std::string& language) const;

    bool use_colors_ = true;
    std::string buffer_;  // Streaming buffer for partial lines
};

}  // namespace ccmake
