#include "terminal/input.hpp"
#include <algorithm>

namespace ccmake {

// Helper to append a vector of tokens to another
static void append_tokens(std::vector<AnsiTokenizer::Token>& dst, std::vector<AnsiTokenizer::Token>&& src) {
    dst.insert(dst.end(), src.begin(), src.end());
}

// --- AnsiTokenizer ---

bool AnsiTokenizer::is_csi_final(unsigned char c) const {
    return c >= 0x40 && c <= 0x7E;
}

bool AnsiTokenizer::is_csi_param(unsigned char c) const {
    return (c >= 0x30 && c <= 0x3F); // digits, ;, <, =, >, ?
}

bool AnsiTokenizer::is_intermediate(unsigned char c) const {
    return c >= 0x20 && c <= 0x2F;
}

std::vector<AnsiTokenizer::Token> AnsiTokenizer::emit_text() {
    std::vector<Token> tokens;
    if (!text_buf_.empty()) {
        tokens.push_back({Token::Text, text_buf_});
        text_buf_.clear();
    }
    return tokens;
}

std::vector<AnsiTokenizer::Token> AnsiTokenizer::emit_sequence() {
    std::vector<Token> tokens = emit_text();
    if (!buffer_.empty()) {
        tokens.push_back({Token::Sequence, buffer_});
        buffer_.clear();
    }
    return tokens;
}

std::vector<AnsiTokenizer::Token> AnsiTokenizer::feed(const std::string& input) {
    std::vector<Token> tokens;

    for (unsigned char c : input) {
        switch (state_) {
            case State::Ground:
                if (c == 0x1B) {
                    // ESC -- flush accumulated text, enter Escape state
                    append_tokens(tokens, emit_text());
                    state_ = State::Escape;
                    buffer_.clear();
                    buffer_ += static_cast<char>(c);
                } else {
                    text_buf_ += static_cast<char>(c);
                }
                break;

            case State::Escape:
                buffer_ += static_cast<char>(c);
                if (c == '[') {
                    state_ = State::Csi;
                } else if (c == ']') {
                    state_ = State::Osc;
                } else if (c == 'O') {
                    state_ = State::Ss3;
                } else if (c == 'P') {
                    state_ = State::Dcs;
                } else if (c == '_') {
                    state_ = State::Apc;
                } else if (is_intermediate(c)) {
                    state_ = State::EscapeIntermediate;
                } else if ((c >= 0x40 && c <= 0x5F) || (c >= 0x61 && c <= 0x7A)) {
                    // Single-char escape sequence (e.g. ESC c, ESC O)
                    append_tokens(tokens, emit_sequence());
                    state_ = State::Ground;
                } else {
                    // Unknown, treat as complete
                    append_tokens(tokens, emit_sequence());
                    state_ = State::Ground;
                }
                break;

            case State::Csi:
                buffer_ += static_cast<char>(c);
                if (is_csi_final(c)) {
                    append_tokens(tokens, emit_sequence());
                    state_ = State::Ground;
                }
                // Otherwise keep accumulating params
                break;

            case State::Osc:
                buffer_ += static_cast<char>(c);
                if (c == 0x07) {
                    // BEL terminates OSC
                    append_tokens(tokens, emit_sequence());
                    state_ = State::Ground;
                }
                break;

            case State::Ss3:
                buffer_ += static_cast<char>(c);
                if (c >= 0x40 && c <= 0x7E) {
                    append_tokens(tokens, emit_sequence());
                    state_ = State::Ground;
                }
                break;

            case State::Dcs:
                buffer_ += static_cast<char>(c);
                if (c == 0x07) {
                    append_tokens(tokens, emit_sequence());
                    state_ = State::Ground;
                }
                break;

            case State::Apc:
                buffer_ += static_cast<char>(c);
                if (c == 0x07) {
                    append_tokens(tokens, emit_sequence());
                    state_ = State::Ground;
                }
                break;

            case State::EscapeIntermediate:
                buffer_ += static_cast<char>(c);
                if (c >= 0x30 && c <= 0x7E) {
                    // Final byte
                    append_tokens(tokens, emit_sequence());
                    state_ = State::Ground;
                }
                break;
        }
    }

    // Handle OSC terminated by ST (ESC \) -- check if buffer ends with ESC backslash
    if (state_ == State::Osc && buffer_.size() >= 2) {
        size_t sz = buffer_.size();
        if (static_cast<unsigned char>(buffer_[sz - 2]) == 0x1B && buffer_[sz - 1] == '\\') {
            append_tokens(tokens, emit_sequence());
            state_ = State::Ground;
        }
    }

    return tokens;
}

std::vector<AnsiTokenizer::Token> AnsiTokenizer::flush() {
    std::vector<Token> tokens = emit_text();
    // Any buffered escape sequence that wasn't completed gets emitted
    if (!buffer_.empty()) {
        tokens.push_back({Token::Sequence, buffer_});
        buffer_.clear();
    }
    state_ = State::Ground;
    return tokens;
}

void AnsiTokenizer::reset() {
    state_ = State::Ground;
    buffer_.clear();
    text_buf_.clear();
}

// --- parse_keypress helper ---

static ParsedKey make_key(const std::string& name, const std::string& raw,
                           const std::string& sequence) {
    ParsedKey k;
    k.name = name;
    k.raw = raw;
    k.sequence = sequence;
    return k;
}

// --- parse_keypress ---

ParseResult parse_keypress(const std::string& input, bool /*is_paste_mode*/) {
    ParseResult result;

    if (input.empty()) {
        result.remaining = "";
        return result;
    }

    unsigned char first = static_cast<unsigned char>(input[0]);

    // ESC byte
    if (first == 0x1B) {
        if (input.size() == 1) {
            // Just ESC by itself
            result.inputs.push_back(make_key("escape", input, ""));
            result.remaining = "";
            return result;
        }

        // ESC[... -- CSI sequence
        if (input[1] == '[' && input.size() >= 3) {
            // Find the final byte
            size_t end = 2;
            while (end < input.size()) {
                unsigned char c = static_cast<unsigned char>(input[end]);
                if (c >= 0x40 && c <= 0x7E) break;
                ++end;
            }

            if (end >= input.size()) {
                // Incomplete CSI sequence
                result.remaining = input;
                return result;
            }

            std::string seq = input.substr(0, end + 1);
            unsigned char final_byte = static_cast<unsigned char>(input[end]);
            std::string params = input.substr(2, end - 2);

            ParsedKey key;
            key.sequence = seq;
            key.raw = seq;

            if (final_byte == 'A') key.name = "up";
            else if (final_byte == 'B') key.name = "down";
            else if (final_byte == 'C') key.name = "right";
            else if (final_byte == 'D') key.name = "left";
            else if (final_byte == 'H') key.name = "home";
            else if (final_byte == 'F') key.name = "end";
            else if (final_byte == '~') {
                // CSI parameter sequences
                if (params == "1" || params == "7") key.name = "home";
                else if (params == "2") key.name = "insert";
                else if (params == "3") key.name = "delete";
                else if (params == "4" || params == "8") key.name = "end";
                else if (params == "5") key.name = "pageup";
                else if (params == "6") key.name = "pagedown";
                else if (params == "11") { key.name = "f1"; key.fn = true; }
                else if (params == "12") { key.name = "f2"; key.fn = true; }
                else if (params == "13") { key.name = "f3"; key.fn = true; }
                else if (params == "14") { key.name = "f4"; key.fn = true; }
                else if (params == "15") { key.name = "f5"; key.fn = true; }
                else if (params == "17") { key.name = "f6"; key.fn = true; }
                else if (params == "18") { key.name = "f7"; key.fn = true; }
                else if (params == "19") { key.name = "f8"; key.fn = true; }
                else if (params == "20") { key.name = "f9"; key.fn = true; }
                else if (params == "21") { key.name = "f10"; key.fn = true; }
                else if (params == "23") { key.name = "f11"; key.fn = true; }
                else if (params == "24") { key.name = "f12"; key.fn = true; }
                else key.name = "unknown";
            } else {
                key.name = "unknown";
            }

            result.inputs.push_back(key);
            result.remaining = input.substr(end + 1);
            return result;
        }

        // ESC O followed by a letter -- SS3 function keys
        if (input[1] == 'O' && input.size() >= 3) {
            unsigned char letter = static_cast<unsigned char>(input[2]);
            ParsedKey key;
            key.raw = input.substr(0, 3);
            key.sequence = key.raw;
            key.fn = true;

            if (letter == 'A') key.name = "f1";
            else if (letter == 'B') key.name = "f2";
            else if (letter == 'C') key.name = "f3";
            else if (letter == 'D') key.name = "f4";
            else if (letter == 'P') key.name = "f1";
            else if (letter == 'Q') key.name = "f2";
            else if (letter == 'R') key.name = "f3";
            else if (letter == 'S') key.name = "f4";
            else key.name = "unknown";

            result.inputs.push_back(key);
            result.remaining = input.substr(3);
            return result;
        }

        // ESC followed by a printable character -- Meta (Alt+key)
        if (input.size() >= 2) {
            unsigned char next = static_cast<unsigned char>(input[1]);
            if (next >= 0x20 && next <= 0x7E) {
                ParsedKey key;
                key.meta = true;
                key.raw = input.substr(0, 2);
                if (next >= 'a' && next <= 'z') {
                    key.name = std::string(1, static_cast<char>(next));
                } else if (next >= 'A' && next <= 'Z') {
                    key.name = std::string(1, static_cast<char>(next + 32)); // lowercase
                    key.shift = true;
                } else if (next >= '0' && next <= '9') {
                    key.name = std::string(1, static_cast<char>(next));
                } else {
                    key.name = std::string(1, static_cast<char>(next));
                }
                result.inputs.push_back(key);
                result.remaining = input.substr(2);
                return result;
            }
        }

        // Fallback: escape key
        result.inputs.push_back(make_key("escape", "\x1b", ""));
        result.remaining = input.substr(1);
        return result;
    }

    // Special control characters with dedicated names
    if (first == 0x0D || first == 0x0A) {
        // CR or LF -- return
        result.inputs.push_back(make_key("return", input.substr(0, 1), ""));
        result.remaining = input.substr(1);
        return result;
    }

    if (first == 0x09) {
        // Tab
        result.inputs.push_back(make_key("tab", input.substr(0, 1), ""));
        result.remaining = input.substr(1);
        return result;
    }

    if (first == 0x7F || first == 0x08) {
        // DEL or BS -- backspace
        result.inputs.push_back(make_key("backspace", input.substr(0, 1), ""));
        result.remaining = input.substr(1);
        return result;
    }

    // Control characters: Ctrl+A (0x01) through Ctrl+Z (0x1A)
    // Excluding Tab(0x09), LF(0x0A), CR(0x0D) which are handled above
    if (first >= 0x01 && first <= 0x1A && first != 0x09 && first != 0x0A && first != 0x0D) {
        char letter = static_cast<char>('a' + first - 0x01);
        ParsedKey key;
        key.ctrl = true;
        key.name = std::string(1, letter);
        key.raw = input.substr(0, 1);
        result.inputs.push_back(key);
        result.remaining = input.substr(1);
        return result;
    }

    if (first == 0x1C) {
        result.inputs.push_back(make_key("\\", input.substr(0, 1), ""));
        result.remaining = input.substr(1);
        return result;
    }
    if (first == 0x1D) {
        result.inputs.push_back(make_key("]", input.substr(0, 1), ""));
        result.remaining = input.substr(1);
        return result;
    }
    if (first == 0x1E) {
        result.inputs.push_back(make_key("^", input.substr(0, 1), ""));
        result.remaining = input.substr(1);
        return result;
    }
    if (first == 0x1F) {
        result.inputs.push_back(make_key("_", input.substr(0, 1), ""));
        result.remaining = input.substr(1);
        return result;
    }

    // Printable character
    if (first >= 0x20 && first <= 0x7E) {
        if (first >= 'A' && first <= 'Z') {
            // Uppercase letter: store lowercase with shift
            ParsedKey key;
            key.name = std::string(1, static_cast<char>(first + 32));
            key.shift = true;
            key.raw = input.substr(0, 1);
            result.inputs.push_back(key);
        } else {
            result.inputs.push_back(make_key(std::string(1, static_cast<char>(first)), input.substr(0, 1), ""));
        }
        result.remaining = input.substr(1);
        return result;
    }

    // UTF-8 multi-byte or other -- pass through as-is
    if (first >= 0x80) {
        // Determine UTF-8 sequence length
        int len = 1;
        if ((first & 0xE0) == 0xC0) len = 2;
        else if ((first & 0xF0) == 0xE0) len = 3;
        else if ((first & 0xF8) == 0xF0) len = 4;

        if (static_cast<int>(input.size()) < len) {
            result.remaining = input;
            return result;
        }

        std::string ch = input.substr(0, len);
        ParsedKey key;
        key.name = ch;
        key.raw = ch;
        result.inputs.push_back(key);
        result.remaining = input.substr(len);
        return result;
    }

    // Unknown -- pass through
    result.inputs.push_back(make_key("?", input.substr(0, 1), ""));
    result.remaining = input.substr(1);
    return result;
}

bool looks_like_escape(const std::string& input) {
    if (input.empty()) return false;
    unsigned char first = static_cast<unsigned char>(input[0]);
    return first == 0x1B;
}

}  // namespace ccmake
