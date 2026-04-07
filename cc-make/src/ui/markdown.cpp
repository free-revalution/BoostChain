#include "ui/markdown.hpp"
#include "terminal/ansi.hpp"
#include <sstream>
#include <algorithm>

namespace ccmake {

MarkdownRenderer::MarkdownRenderer() = default;

std::vector<MarkdownBlock> MarkdownRenderer::parse(const std::string& markdown) const {
    std::vector<MarkdownBlock> blocks;
    std::istringstream stream(markdown);
    std::string line;
    bool in_code_block = false;
    std::string code_language;
    std::string code_content;
    int list_number = 0;

    auto flush_code = [&]() {
        if (in_code_block) {
            blocks.push_back({MarkdownBlock::Type::CodeBlock, code_content, code_language});
            code_content.clear();
            code_language.clear();
            in_code_block = false;
        }
    };

    while (std::getline(stream, line)) {
        // Inside a code block, collect lines until closing fence
        if (in_code_block) {
            std::string lang;
            if (is_code_fence(line, lang)) {
                flush_code();
                continue;
            }
            if (!code_content.empty()) {
                code_content += "\n";
            }
            code_content += line;
            continue;
        }

        // Check for code fence (opens code block)
        std::string lang;
        if (is_code_fence(line, lang)) {
            flush_code();  // safety: close any open block
            in_code_block = true;
            code_language = lang;
            continue;
        }

        // Check for heading
        int heading_level = 0;
        if (is_heading(line, heading_level)) {
            flush_code();
            std::string content = line.substr(static_cast<size_t>(heading_level) + 1);
            // Trim leading space
            if (!content.empty() && content[0] == ' ') {
                content = content.substr(1);
            }
            switch (heading_level) {
                case 1: blocks.push_back({MarkdownBlock::Type::Heading1, content, {}}); break;
                case 2: blocks.push_back({MarkdownBlock::Type::Heading2, content, {}}); break;
                case 3: blocks.push_back({MarkdownBlock::Type::Heading3, content, {}}); break;
            }
            list_number = 0;
            continue;
        }

        // Horizontal rule: --- or *** (must be at least 3 chars)
        if ((line == "---" || line == "***" || line == "___") ||
            (line.size() >= 3 && line.find_first_not_of("-*_") == std::string::npos)) {
            // Only treat as HR if it's a standalone line of only those chars and at least 3 long
            if (line.size() >= 3) {
                bool all_hr_chars = true;
                for (char c : line) {
                    if (c != '-' && c != '*' && c != '_') {
                        all_hr_chars = false;
                        break;
                    }
                }
                if (all_hr_chars) {
                    flush_code();
                    blocks.push_back({MarkdownBlock::Type::HorizontalRule, line, {}});
                    list_number = 0;
                    continue;
                }
            }
        }

        // Blank line
        if (line.empty() || (line.size() == 1 && line[0] == '\r')) {
            flush_code();
            list_number = 0;
            blocks.push_back({MarkdownBlock::Type::BlankLine, "", {}});
            continue;
        }

        // Bullet list: "- " or "* " at start
        if ((line.size() >= 2) &&
            ((line[0] == '-' || line[0] == '*') && line[1] == ' ')) {
            flush_code();
            std::string content = line.substr(2);
            blocks.push_back({MarkdownBlock::Type::BulletList, content, {}});
            list_number = 0;
            continue;
        }

        // Numbered list: "N. " at start
        if (line.size() >= 3) {
            bool is_numbered = true;
            size_t dot_pos = 0;
            for (size_t i = 0; i < line.size(); i++) {
                if (line[i] == '.' && i > 0 && i + 1 < line.size() && line[i + 1] == ' ') {
                    dot_pos = i;
                    break;
                }
                if (!std::isdigit(static_cast<unsigned char>(line[i]))) {
                    is_numbered = false;
                    break;
                }
            }
            if (is_numbered && dot_pos > 0) {
                std::string num_str = line.substr(0, dot_pos);
                std::string content = line.substr(dot_pos + 2);
                blocks.push_back({MarkdownBlock::Type::NumberedList, content, {}});
                list_number++;
                continue;
            }
        }

        // Blockquote: "> " at start
        if (line.size() >= 2 && line[0] == '>' && line[1] == ' ') {
            flush_code();
            std::string content = line.substr(2);
            blocks.push_back({MarkdownBlock::Type::Blockquote, content, {}});
            list_number = 0;
            continue;
        }

        // Regular text
        flush_code();
        list_number = 0;
        blocks.push_back({MarkdownBlock::Type::Text, line, {}});
    }

    // Flush any remaining code block
    flush_code();

    // If the input was empty (or only whitespace with no newline),
    // we never entered the loop, so produce a BlankLine block.
    if (blocks.empty() && markdown.empty()) {
        blocks.push_back({MarkdownBlock::Type::BlankLine, "", {}});
    }

    return blocks;
}

bool MarkdownRenderer::is_heading(const std::string& line, int& level) const {
    if (line.empty()) return false;

    int count = 0;
    for (size_t i = 0; i < line.size() && i < 6; i++) {
        if (line[i] == '#') {
            count++;
        } else {
            break;
        }
    }

    // Must have 1-3 '#' followed by a space
    if (count >= 1 && count <= 3 && line.size() > static_cast<size_t>(count) && line[count] == ' ') {
        level = count;
        return true;
    }

    return false;
}

bool MarkdownRenderer::is_code_fence(const std::string& line, std::string& language) const {
    if (line.size() < 3) return false;

    if ((line[0] == '`' && line[1] == '`' && line[2] == '`') ||
        (line[0] == '~' && line[1] == '~' && line[2] == '~')) {
        // Extract language if present
        if (line.size() > 3 && line[3] != ' ' && line[3] != '\0') {
            // Skip one optional space
            size_t lang_start = 3;
            if (lang_start < line.size() && line[lang_start] == ' ') {
                lang_start++;
            }
            language = line.substr(lang_start);
        } else {
            language.clear();
        }
        return true;
    }

    return false;
}

std::string MarkdownRenderer::render_inline(const std::string& text) const {
    std::string result;
    size_t i = 0;

    while (i < text.size()) {
        // Bold: **text**
        if (i + 1 < text.size() && text[i] == '*' && text[i + 1] == '*') {
            size_t end = text.find("**", i + 2);
            if (end != std::string::npos) {
                std::string inner = text.substr(i + 2, end - i - 2);
                if (use_colors_) {
                    result += ansi::bold() + inner + ansi::reset();
                } else {
                    result += inner;
                }
                i = end + 2;
                continue;
            }
        }

        // Italic: *text* (but not **)
        if (text[i] == '*' && (i + 1 < text.size() && text[i + 1] != '*') &&
            (i == 0 || text[i - 1] != '*')) {
            size_t end = text.find('*', i + 1);
            if (end != std::string::npos && end + 1 < text.size() && text[end + 1] != '*') {
                // Also make sure it's not preceded by * (would be part of bold)
                std::string inner = text.substr(i + 1, end - i - 1);
                if (!inner.empty()) {
                    if (use_colors_) {
                        result += ansi::italic() + inner + ansi::reset();
                    } else {
                        result += inner;
                    }
                    i = end + 1;
                    continue;
                }
            }
        }

        // Inline code: `text`
        if (text[i] == '`') {
            size_t end = text.find('`', i + 1);
            if (end != std::string::npos) {
                std::string inner = text.substr(i + 1, end - i - 1);
                if (use_colors_) {
                    result += ansi::inverse() + inner + ansi::reset();
                } else {
                    result += "`" + inner + "`";
                }
                i = end + 1;
                continue;
            }
        }

        // Link: [text](url)
        if (text[i] == '[') {
            size_t bracket_end = text.find(']', i + 1);
            if (bracket_end != std::string::npos &&
                bracket_end + 1 < text.size() && text[bracket_end + 1] == '(') {
                size_t paren_end = text.find(')', bracket_end + 2);
                if (paren_end != std::string::npos) {
                    std::string link_text = text.substr(i + 1, bracket_end - i - 1);
                    std::string url = text.substr(bracket_end + 2, paren_end - bracket_end - 2);
                    if (use_colors_) {
                        result += link_text + " (" + ansi::underline() + url + ansi::reset() + ")";
                    } else {
                        result += link_text + " (" + url + ")";
                    }
                    i = paren_end + 1;
                    continue;
                }
            }
        }

        result += text[i];
        i++;
    }

    return result;
}

std::string MarkdownRenderer::render(const std::vector<MarkdownBlock>& blocks) const {
    std::string output;

    for (const auto& block : blocks) {
        switch (block.type) {
            case MarkdownBlock::Type::Heading1:
                if (use_colors_) {
                    output += ansi::bold() + ansi::fg_bright_white() + block.content + ansi::reset() + "\n";
                } else {
                    output += block.content + "\n";
                }
                break;

            case MarkdownBlock::Type::Heading2:
                if (use_colors_) {
                    output += ansi::bold() + ansi::fg_bright_cyan() + block.content + ansi::reset() + "\n";
                } else {
                    output += block.content + "\n";
                }
                break;

            case MarkdownBlock::Type::Heading3:
                if (use_colors_) {
                    output += ansi::bold() + ansi::fg_cyan() + block.content + ansi::reset() + "\n";
                } else {
                    output += block.content + "\n";
                }
                break;

            case MarkdownBlock::Type::CodeBlock: {
                std::string border(use_colors_ ? ansi::dim() : "", 0);
                if (use_colors_) {
                    border = ansi::fg_bright_black();
                }
                std::string reset = use_colors_ ? ansi::reset() : "";

                output += border + "+";
                for (int i = 0; i < 40; ++i) output += "-";
                if (!block.language.empty()) {
                    output += " " + block.language;
                }
                output += reset + "\n";

                // Render code content line by line
                std::istringstream code_stream(block.content);
                std::string code_line;
                while (std::getline(code_stream, code_line)) {
                    output += border + "| " + code_line + reset + "\n";
                }

                output += border + "+";
                for (int i = 0; i < 40; ++i) output += "-";
                output += reset + "\n";
                break;
            }

            case MarkdownBlock::Type::InlineCode:
                if (use_colors_) {
                    output += ansi::inverse() + block.content + ansi::reset();
                } else {
                    output += "`" + block.content + "`";
                }
                break;

            case MarkdownBlock::Type::Bold:
                if (use_colors_) {
                    output += ansi::bold() + block.content + ansi::reset();
                } else {
                    output += block.content;
                }
                break;

            case MarkdownBlock::Type::Italic:
                if (use_colors_) {
                    output += ansi::italic() + block.content + ansi::reset();
                } else {
                    output += block.content;
                }
                break;

            case MarkdownBlock::Type::BulletList:
                output += "  \xe2\x80\xa2 " + render_inline(block.content) + "\n";
                break;

            case MarkdownBlock::Type::NumberedList:
                output += "  " + render_inline(block.content) + "\n";
                break;

            case MarkdownBlock::Type::Blockquote: {
                std::string prefix = use_colors_ ? (ansi::fg_bright_black() + "\xe2\x94\x82 " + ansi::reset()) : "> ";
                output += prefix + render_inline(block.content) + "\n";
                break;
            }

            case MarkdownBlock::Type::HorizontalRule: {
                std::string hr_color = use_colors_ ? ansi::fg_bright_black() : "";
                std::string hr_reset = use_colors_ ? ansi::reset() : "";
                output += hr_color + "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80" + hr_reset + "\n";
                break;
            }

            case MarkdownBlock::Type::Link:
                if (use_colors_) {
                    output += block.content + " (" + ansi::underline() + block.language + ansi::reset() + ")\n";
                } else {
                    output += block.content + " (" + block.language + ")\n";
                }
                break;

            case MarkdownBlock::Type::BlankLine:
                output += "\n";
                break;

            case MarkdownBlock::Type::Text:
                output += render_inline(block.content) + "\n";
                break;
        }
    }

    return output;
}

std::string MarkdownRenderer::render_string(const std::string& markdown) const {
    auto blocks = parse(markdown);
    return render(blocks);
}

std::vector<std::string> MarkdownRenderer::feed(const std::string& partial) {
    buffer_ += partial;
    std::vector<std::string> lines;

    // Split into complete lines
    size_t pos = 0;
    while ((pos = buffer_.find('\n')) != std::string::npos) {
        std::string line = buffer_.substr(0, pos);
        // Remove trailing \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
        buffer_.erase(0, pos + 1);
    }

    // Parse accumulated complete lines and render them
    // Join lines back with newlines, parse, render, then split back
    if (!lines.empty()) {
        std::string joined;
        for (size_t i = 0; i < lines.size(); i++) {
            if (i > 0) joined += "\n";
            joined += lines[i];
        }
        std::string rendered = render_string(joined);

        // Split rendered output into lines
        std::vector<std::string> result;
        std::istringstream stream(rendered);
        std::string out_line;
        while (std::getline(stream, out_line)) {
            result.push_back(out_line);
        }
        return result;
    }

    return {};
}

void MarkdownRenderer::reset() {
    buffer_.clear();
}

void MarkdownRenderer::set_use_colors(bool use) {
    use_colors_ = use;
}

bool MarkdownRenderer::use_colors() const {
    return use_colors_;
}

}  // namespace ccmake
