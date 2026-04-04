#pragma once
#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <variant>

namespace ccmake {

enum class InputKind { Key, Mouse, Response };

struct ParsedKey {
    std::string name;           // 'up', 'down', 'return', 'escape', 'tab', 'backspace', 'space', 'a'-'z', '0'-'9', etc.
    bool ctrl = false;
    bool meta = false;          // Alt/Option
    bool shift = false;
    bool fn = false;            // Function key
    std::string sequence;       // Raw escape sequence (for function keys)
    std::string raw;            // Raw input bytes

    InputKind kind = InputKind::Key;
};

struct ParsedMouse {
    InputKind kind = InputKind::Mouse;
    int button = 0;             // 0=left, 1=middle, 2=right
    std::string action;         // "press", "release"
    int col = 0, row = 0;
    std::string sequence;
};

using ParsedInput = std::variant<ParsedKey, ParsedMouse>;

// ANSI escape sequence tokenizer
class AnsiTokenizer {
public:
    enum class State { Ground, Escape, Csi, Osc, Ss3, Dcs, Apc, EscapeIntermediate };

    struct Token {
        enum Type { Text, Sequence } type;
        std::string value;
    };

    std::vector<Token> feed(const std::string& input);
    std::vector<Token> flush();
    void reset();
    const std::string& buffer() const { return buffer_; }

private:
    State state_ = State::Ground;
    std::string buffer_;
    std::string text_buf_;

    std::vector<Token> emit_text();
    std::vector<Token> emit_sequence();
    bool is_csi_final(unsigned char c) const;
    bool is_csi_param(unsigned char c) const;
    bool is_intermediate(unsigned char c) const;
};

// Parse keypress from raw terminal input
// Returns parsed inputs and remaining unprocessed bytes
struct ParseResult {
    std::vector<ParsedInput> inputs;
    std::string remaining;
};

ParseResult parse_keypress(const std::string& input, bool is_paste_mode = false);

// Check if input looks like the start of an escape sequence
bool looks_like_escape(const std::string& input);

}  // namespace ccmake
