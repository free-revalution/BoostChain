#include <catch2/catch_test_macros.hpp>
#include "terminal/input.hpp"

using namespace ccmake;

// ============================================================================
// AnsiTokenizer tests
// ============================================================================

TEST_CASE("AnsiTokenizer: plain text accumulated by feed, emitted by flush", "[input][tokenizer]") {
    AnsiTokenizer tok;
    auto tokens = tok.feed("hello");
    // feed() buffers plain text; it is emitted on flush()
    REQUIRE(tokens.empty());

    auto flushed = tok.flush();
    REQUIRE(flushed.size() == 1);
    REQUIRE(flushed[0].type == AnsiTokenizer::Token::Text);
    REQUIRE(flushed[0].value == "hello");
}

TEST_CASE("AnsiTokenizer: ESC[A produces Sequence token", "[input][tokenizer]") {
    AnsiTokenizer tok;
    auto tokens = tok.feed("\x1b[A");
    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0].type == AnsiTokenizer::Token::Sequence);
    REQUIRE(tokens[0].value == "\x1b[A");
}

TEST_CASE("AnsiTokenizer: ESC[A with leading text produces Text then Sequence", "[input][tokenizer]") {
    AnsiTokenizer tok;
    auto tokens = tok.feed("hello\x1b[A");
    REQUIRE(tokens.size() == 2);
    REQUIRE(tokens[0].type == AnsiTokenizer::Token::Text);
    REQUIRE(tokens[0].value == "hello");
    REQUIRE(tokens[1].type == AnsiTokenizer::Token::Sequence);
    REQUIRE(tokens[1].value == "\x1b[A");
}

TEST_CASE("AnsiTokenizer: multiple plain characters accumulated and flushed", "[input][tokenizer]") {
    AnsiTokenizer tok;
    auto tokens = tok.feed("abcdef");
    // Plain text stays buffered until flush()
    REQUIRE(tokens.empty());

    auto flushed = tok.flush();
    REQUIRE(flushed.size() == 1);
    REQUIRE(flushed[0].type == AnsiTokenizer::Token::Text);
    REQUIRE(flushed[0].value == "abcdef");
}

TEST_CASE("AnsiTokenizer: incomplete CSI sequence stays in buffer", "[input][tokenizer]") {
    AnsiTokenizer tok;
    auto tokens = tok.feed("\x1b[");
    REQUIRE(tokens.empty());
    REQUIRE_FALSE(tok.buffer().empty());
}

TEST_CASE("AnsiTokenizer: incomplete CSI sequence completed by second feed", "[input][tokenizer]") {
    AnsiTokenizer tok;
    auto t1 = tok.feed("\x1b[");
    REQUIRE(t1.empty());

    auto t2 = tok.feed("A");
    REQUIRE(t2.size() == 1);
    REQUIRE(t2[0].type == AnsiTokenizer::Token::Sequence);
    REQUIRE(t2[0].value == "\x1b[A");
}

TEST_CASE("AnsiTokenizer: OSC terminated by BEL", "[input][tokenizer]") {
    AnsiTokenizer tok;
    auto tokens = tok.feed("\x1b]0;title\x07");
    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0].type == AnsiTokenizer::Token::Sequence);
    REQUIRE(tokens[0].value == "\x1b]0;title\x07");
}

TEST_CASE("AnsiTokenizer: OSC terminated by ST (ESC \\)", "[input][tokenizer]") {
    AnsiTokenizer tok;
    auto tokens = tok.feed("\x1b]0;title\x1b\\");
    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0].type == AnsiTokenizer::Token::Sequence);
    REQUIRE(tokens[0].value == "\x1b]0;title\x1b\\");
}

TEST_CASE("AnsiTokenizer: flush() emits buffered content", "[input][tokenizer]") {
    AnsiTokenizer tok;
    tok.feed("\x1b[");
    // Incomplete CSI is in the buffer
    REQUIRE_FALSE(tok.buffer().empty());

    auto tokens = tok.flush();
    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0].type == AnsiTokenizer::Token::Sequence);
    REQUIRE(tokens[0].value == "\x1b[");

    // Buffer should be clear after flush
    REQUIRE(tok.buffer().empty());
}

TEST_CASE("AnsiTokenizer: flush() emits buffered text", "[input][tokenizer]") {
    AnsiTokenizer tok;
    tok.feed("abc");
    auto tokens = tok.flush();
    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0].type == AnsiTokenizer::Token::Text);
    REQUIRE(tokens[0].value == "abc");
}

TEST_CASE("AnsiTokenizer: reset() clears state", "[input][tokenizer]") {
    AnsiTokenizer tok;
    tok.feed("\x1b[");
    REQUIRE_FALSE(tok.buffer().empty());

    tok.reset();
    REQUIRE(tok.buffer().empty());

    // After reset, new input should be buffered (text emitted only on flush)
    auto tokens = tok.feed("hello");
    REQUIRE(tokens.empty());

    auto flushed = tok.flush();
    REQUIRE(flushed.size() == 1);
    REQUIRE(flushed[0].type == AnsiTokenizer::Token::Text);
    REQUIRE(flushed[0].value == "hello");
}

TEST_CASE("AnsiTokenizer: SS3 sequence produces Sequence token", "[input][tokenizer]") {
    AnsiTokenizer tok;
    auto tokens = tok.feed("\x1bOP");
    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0].type == AnsiTokenizer::Token::Sequence);
    REQUIRE(tokens[0].value == "\x1bOP");
}

TEST_CASE("AnsiTokenizer: multiple sequences separated by text", "[input][tokenizer]") {
    AnsiTokenizer tok;
    auto tokens = tok.feed("\x1b[Ahello\x1b[B");
    REQUIRE(tokens.size() == 3);
    REQUIRE(tokens[0].type == AnsiTokenizer::Token::Sequence);
    REQUIRE(tokens[0].value == "\x1b[A");
    REQUIRE(tokens[1].type == AnsiTokenizer::Token::Text);
    REQUIRE(tokens[1].value == "hello");
    REQUIRE(tokens[2].type == AnsiTokenizer::Token::Sequence);
    REQUIRE(tokens[2].value == "\x1b[B");
}

TEST_CASE("AnsiTokenizer: DCS terminated by BEL", "[input][tokenizer]") {
    AnsiTokenizer tok;
    auto tokens = tok.feed("\x1bPtest\x07");
    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0].type == AnsiTokenizer::Token::Sequence);
    REQUIRE(tokens[0].value == "\x1bPtest\x07");
}

// ============================================================================
// parse_keypress tests - empty string
// ============================================================================

TEST_CASE("parse_keypress: empty string returns no inputs", "[input]") {
    auto result = parse_keypress("");
    REQUIRE(result.inputs.empty());
    REQUIRE(result.remaining.empty());
}

// ============================================================================
// parse_keypress tests - ESC alone
// ============================================================================

TEST_CASE("parse_keypress: ESC produces name='escape'", "[input]") {
    auto result = parse_keypress("\x1b");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "escape");
    REQUIRE_FALSE(key->ctrl);
    REQUIRE_FALSE(key->meta);
    REQUIRE_FALSE(key->shift);
    REQUIRE(result.remaining.empty());
}

// ============================================================================
// parse_keypress tests - arrow keys
// ============================================================================

TEST_CASE("parse_keypress: ESC[A produces name='up'", "[input]") {
    auto result = parse_keypress("\x1b[A");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "up");
    REQUIRE(key->sequence == "\x1b[A");
    REQUIRE(result.remaining.empty());
}

TEST_CASE("parse_keypress: ESC[B produces name='down'", "[input]") {
    auto result = parse_keypress("\x1b[B");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "down");
    REQUIRE(key->sequence == "\x1b[B");
    REQUIRE(result.remaining.empty());
}

TEST_CASE("parse_keypress: ESC[C produces name='right'", "[input]") {
    auto result = parse_keypress("\x1b[C");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "right");
    REQUIRE(key->sequence == "\x1b[C");
    REQUIRE(result.remaining.empty());
}

TEST_CASE("parse_keypress: ESC[D produces name='left'", "[input]") {
    auto result = parse_keypress("\x1b[D");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "left");
    REQUIRE(key->sequence == "\x1b[D");
    REQUIRE(result.remaining.empty());
}

// ============================================================================
// parse_keypress tests - home and end (H/F)
// ============================================================================

TEST_CASE("parse_keypress: ESC[H produces name='home'", "[input]") {
    auto result = parse_keypress("\x1b[H");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "home");
    REQUIRE(key->sequence == "\x1b[H");
}

TEST_CASE("parse_keypress: ESC[F produces name='end'", "[input]") {
    auto result = parse_keypress("\x1b[F");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "end");
    REQUIRE(key->sequence == "\x1b[F");
}

// ============================================================================
// parse_keypress tests - CSI ~ parameter sequences
// ============================================================================

TEST_CASE("parse_keypress: ESC[1~ produces home", "[input]") {
    auto result = parse_keypress("\x1b[1~");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "home");
    REQUIRE(key->sequence == "\x1b[1~");
}

TEST_CASE("parse_keypress: ESC[7~ produces home", "[input]") {
    auto result = parse_keypress("\x1b[7~");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "home");
}

TEST_CASE("parse_keypress: ESC[4~ produces end", "[input]") {
    auto result = parse_keypress("\x1b[4~");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "end");
}

TEST_CASE("parse_keypress: ESC[8~ produces end", "[input]") {
    auto result = parse_keypress("\x1b[8~");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "end");
}

TEST_CASE("parse_keypress: ESC[2~ produces insert", "[input]") {
    auto result = parse_keypress("\x1b[2~");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "insert");
}

TEST_CASE("parse_keypress: ESC[3~ produces delete", "[input]") {
    auto result = parse_keypress("\x1b[3~");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "delete");
}

TEST_CASE("parse_keypress: ESC[5~ produces pageup", "[input]") {
    auto result = parse_keypress("\x1b[5~");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "pageup");
}

TEST_CASE("parse_keypress: ESC[6~ produces pagedown", "[input]") {
    auto result = parse_keypress("\x1b[6~");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "pagedown");
}

// ============================================================================
// parse_keypress tests - CSI function keys (11~ through 24~)
// ============================================================================

TEST_CASE("parse_keypress: CSI f1 through f4 (11~ - 14~)", "[input]") {
    auto check = [](const std::string& seq, const std::string& name) {
        auto result = parse_keypress(seq);
        REQUIRE(result.inputs.size() == 1);
        auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
        REQUIRE(key != nullptr);
        REQUIRE(key->name == name);
        REQUIRE(key->fn);
        REQUIRE(key->sequence == seq);
    };

    check("\x1b[11~", "f1");
    check("\x1b[12~", "f2");
    check("\x1b[13~", "f3");
    check("\x1b[14~", "f4");
}

TEST_CASE("parse_keypress: CSI f5 through f10 (15~, 17~ - 20~)", "[input]") {
    auto check = [](const std::string& seq, const std::string& name) {
        auto result = parse_keypress(seq);
        REQUIRE(result.inputs.size() == 1);
        auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
        REQUIRE(key != nullptr);
        REQUIRE(key->name == name);
        REQUIRE(key->fn);
    };

    check("\x1b[15~", "f5");
    check("\x1b[17~", "f6");
    check("\x1b[18~", "f7");
    check("\x1b[19~", "f8");
    check("\x1b[20~", "f9");
    check("\x1b[21~", "f10");
}

TEST_CASE("parse_keypress: CSI f11 and f12 (23~, 24~)", "[input]") {
    auto check = [](const std::string& seq, const std::string& name) {
        auto result = parse_keypress(seq);
        REQUIRE(result.inputs.size() == 1);
        auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
        REQUIRE(key != nullptr);
        REQUIRE(key->name == name);
        REQUIRE(key->fn);
    };

    check("\x1b[23~", "f11");
    check("\x1b[24~", "f12");
}

// ============================================================================
// parse_keypress tests - SS3 function keys
// ============================================================================

TEST_CASE("parse_keypress: SS3 f1-f4 via A-D", "[input]") {
    auto check = [](const std::string& seq, const std::string& name) {
        auto result = parse_keypress(seq);
        REQUIRE(result.inputs.size() == 1);
        auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
        REQUIRE(key != nullptr);
        REQUIRE(key->name == name);
        REQUIRE(key->fn);
        REQUIRE(key->raw == seq);
    };

    check("\x1bOA", "f1");
    check("\x1bOB", "f2");
    check("\x1bOC", "f3");
    check("\x1bOD", "f4");
}

TEST_CASE("parse_keypress: SS3 f1-f4 via P-S", "[input]") {
    auto check = [](const std::string& seq, const std::string& name) {
        auto result = parse_keypress(seq);
        REQUIRE(result.inputs.size() == 1);
        auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
        REQUIRE(key != nullptr);
        REQUIRE(key->name == name);
        REQUIRE(key->fn);
    };

    check("\x1bOP", "f1");
    check("\x1bOQ", "f2");
    check("\x1bOR", "f3");
    check("\x1bOS", "f4");
}

// ============================================================================
// parse_keypress tests - meta keys (ESC + letter)
// ============================================================================

TEST_CASE("parse_keypress: ESC + lowercase letter produces meta key", "[input]") {
    auto result = parse_keypress("\033a");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "a");
    REQUIRE(key->meta);
    REQUIRE_FALSE(key->ctrl);
    REQUIRE_FALSE(key->shift);
}

TEST_CASE("parse_keypress: ESC + uppercase letter produces meta+shift key", "[input]") {
    auto result = parse_keypress("\033A");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "a");
    REQUIRE(key->meta);
    REQUIRE(key->shift);
}

// ============================================================================
// parse_keypress tests - CR, LF, tab, backspace
// ============================================================================

TEST_CASE("parse_keypress: CR produces name='return'", "[input]") {
    auto result = parse_keypress("\x0d");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "return");
    REQUIRE(result.remaining.empty());
}

TEST_CASE("parse_keypress: LF produces name='return'", "[input]") {
    auto result = parse_keypress("\x0a");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "return");
}

TEST_CASE("parse_keypress: tab produces name='tab'", "[input]") {
    auto result = parse_keypress("\x09");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "tab");
    REQUIRE(result.remaining.empty());
}

TEST_CASE("parse_keypress: DEL (0x7F) produces name='backspace'", "[input]") {
    auto result = parse_keypress("\x7f");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "backspace");
}

TEST_CASE("parse_keypress: BS (0x08) produces name='backspace'", "[input]") {
    auto result = parse_keypress("\x08");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "backspace");
}

// ============================================================================
// parse_keypress tests - ctrl keys
// ============================================================================

TEST_CASE("parse_keypress: Ctrl+A (0x01) produces name='a' with ctrl=true", "[input]") {
    auto result = parse_keypress(std::string(1, '\x01'));
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "a");
    REQUIRE(key->ctrl);
    REQUIRE_FALSE(key->meta);
    REQUIRE_FALSE(key->shift);
}

TEST_CASE("parse_keypress: Ctrl+B (0x02) produces name='b' with ctrl=true", "[input]") {
    auto result = parse_keypress(std::string(1, '\x02'));
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "b");
    REQUIRE(key->ctrl);
}

TEST_CASE("parse_keypress: Ctrl+Z (0x1A) produces name='z' with ctrl=true", "[input]") {
    auto result = parse_keypress(std::string(1, '\x1a'));
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "z");
    REQUIRE(key->ctrl);
}

TEST_CASE("parse_keypress: Ctrl+D (0x04) is not excluded by special char handling", "[input]") {
    auto result = parse_keypress(std::string(1, '\x04'));
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "d");
    REQUIRE(key->ctrl);
}

TEST_CASE("parse_keypress: Tab (0x09) is NOT treated as ctrl key", "[input]") {
    // 0x09 is in the 0x01-0x1A range but should be handled as tab first
    auto result = parse_keypress("\x09");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "tab");
    REQUIRE_FALSE(key->ctrl);
}

TEST_CASE("parse_keypress: LF (0x0A) is NOT treated as ctrl key", "[input]") {
    auto result = parse_keypress("\x0a");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "return");
    REQUIRE_FALSE(key->ctrl);
}

TEST_CASE("parse_keypress: CR (0x0D) is NOT treated as ctrl key", "[input]") {
    auto result = parse_keypress("\x0d");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "return");
    REQUIRE_FALSE(key->ctrl);
}

// ============================================================================
// parse_keypress tests - uppercase and lowercase letters
// ============================================================================

TEST_CASE("parse_keypress: uppercase letter has shift=true and lowercase name", "[input]") {
    auto result = parse_keypress("A");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "a");
    REQUIRE(key->shift);
    REQUIRE_FALSE(key->ctrl);
    REQUIRE_FALSE(key->meta);
}

TEST_CASE("parse_keypress: lowercase letter has no modifiers", "[input]") {
    auto result = parse_keypress("a");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "a");
    REQUIRE_FALSE(key->ctrl);
    REQUIRE_FALSE(key->meta);
    REQUIRE_FALSE(key->shift);
}

TEST_CASE("parse_keypress: uppercase Z has shift=true", "[input]") {
    auto result = parse_keypress("Z");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == "z");
    REQUIRE(key->shift);
}

// ============================================================================
// parse_keypress tests - space and digits
// ============================================================================

TEST_CASE("parse_keypress: space character produces name='space'", "[input]") {
    auto result = parse_keypress(" ");
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == " ");
    REQUIRE_FALSE(key->ctrl);
    REQUIRE_FALSE(key->meta);
    REQUIRE_FALSE(key->shift);
}

TEST_CASE("parse_keypress: digit characters pass through as-is", "[input]") {
    for (char d = '0'; d <= '9'; ++d) {
        auto result = parse_keypress(std::string(1, d));
        REQUIRE(result.inputs.size() == 1);

        auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
        REQUIRE(key != nullptr);
        REQUIRE(key->name == std::string(1, d));
        REQUIRE_FALSE(key->ctrl);
        REQUIRE_FALSE(key->meta);
        REQUIRE_FALSE(key->shift);
    }
}

// ============================================================================
// parse_keypress tests - UTF-8 multi-byte characters
// ============================================================================

TEST_CASE("parse_keypress: 3-byte UTF-8 Chinese character passes through", "[input]") {
    // Chinese character meaning "middle" (U+4E2D) encoded as UTF-8: E4 B8 AD
    std::string utf8 = "\xe4\xb8\xad";
    auto result = parse_keypress(utf8);
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == utf8);
    REQUIRE(key->raw == utf8);
    REQUIRE(result.remaining.empty());
}

TEST_CASE("parse_keypress: 2-byte UTF-8 character passes through", "[input]") {
    // Copyright sign (U+00A9) encoded as C2 A9
    std::string utf8 = "\xc2\xa9";
    auto result = parse_keypress(utf8);
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == utf8);
    REQUIRE(result.remaining.empty());
}

TEST_CASE("parse_keypress: 4-byte UTF-8 character passes through", "[input]") {
    // U+1F600 (grinning face) encoded as F0 9F 98 80
    std::string utf8 = "\xf0\x9f\x98\x80";
    auto result = parse_keypress(utf8);
    REQUIRE(result.inputs.size() == 1);

    auto* key = std::get_if<ParsedKey>(&result.inputs[0]);
    REQUIRE(key != nullptr);
    REQUIRE(key->name == utf8);
    REQUIRE(result.remaining.empty());
}

TEST_CASE("parse_keypress: incomplete UTF-8 sequence stays in remaining", "[input]") {
    // Start of a 3-byte sequence but only 2 bytes provided
    std::string incomplete = "\xe4\xb8";
    auto result = parse_keypress(incomplete);
    REQUIRE(result.inputs.empty());
    REQUIRE(result.remaining == incomplete);
}

// ============================================================================
// looks_like_escape tests
// ============================================================================

TEST_CASE("looks_like_escape: ESC prefix returns true", "[input]") {
    REQUIRE(looks_like_escape("\x1b"));
    REQUIRE(looks_like_escape("\x1b[A"));
    REQUIRE(looks_like_escape("\x1b[5~"));
    REQUIRE(looks_like_escape("\x1bOA"));
}

TEST_CASE("looks_like_escape: non-ESC returns false", "[input]") {
    REQUIRE_FALSE(looks_like_escape("a"));
    REQUIRE_FALSE(looks_like_escape("hello"));
    REQUIRE_FALSE(looks_like_escape("\x01"));
    REQUIRE_FALSE(looks_like_escape("\x0d"));
    REQUIRE_FALSE(looks_like_escape(" "));
}

TEST_CASE("looks_like_escape: empty string returns false", "[input]") {
    REQUIRE_FALSE(looks_like_escape(""));
}

// ============================================================================
// Remaining bytes tests - multiple keys in sequence
// ============================================================================

TEST_CASE("parse_keypress: multiple printable keys parsed one at a time via remaining", "[input]") {
    std::string input = "hello";

    auto r1 = parse_keypress(input);
    REQUIRE(r1.inputs.size() == 1);
    auto* k1 = std::get_if<ParsedKey>(&r1.inputs[0]);
    REQUIRE(k1 != nullptr);
    REQUIRE(k1->name == "h");

    auto r2 = parse_keypress(r1.remaining);
    REQUIRE(r2.inputs.size() == 1);
    auto* k2 = std::get_if<ParsedKey>(&r2.inputs[0]);
    REQUIRE(k2 != nullptr);
    REQUIRE(k2->name == "e");

    auto r3 = parse_keypress(r2.remaining);
    REQUIRE(r3.inputs.size() == 1);
    auto* k3 = std::get_if<ParsedKey>(&r3.inputs[0]);
    REQUIRE(k3 != nullptr);
    REQUIRE(k3->name == "l");
}

TEST_CASE("parse_keypress: ESC sequence followed by text, remaining preserves rest", "[input]") {
    // ESC[A followed by "hello"
    std::string input = "\x1b[Ahello";

    auto r1 = parse_keypress(input);
    REQUIRE(r1.inputs.size() == 1);
    auto* k1 = std::get_if<ParsedKey>(&r1.inputs[0]);
    REQUIRE(k1 != nullptr);
    REQUIRE(k1->name == "up");
    REQUIRE(r1.remaining == "hello");

    auto r2 = parse_keypress(r1.remaining);
    REQUIRE(r2.inputs.size() == 1);
    auto* k2 = std::get_if<ParsedKey>(&r2.inputs[0]);
    REQUIRE(k2 != nullptr);
    REQUIRE(k2->name == "h");
    REQUIRE(r2.remaining == "ello");
}

TEST_CASE("parse_keypress: text followed by ESC sequence, remaining preserves rest", "[input]") {
    std::string input = "ab\x1b[C";

    auto r1 = parse_keypress(input);
    REQUIRE(r1.inputs.size() == 1);
    auto* k1 = std::get_if<ParsedKey>(&r1.inputs[0]);
    REQUIRE(k1 != nullptr);
    REQUIRE(k1->name == "a");
    REQUIRE(r1.remaining == "b\x1b[C");

    auto r2 = parse_keypress(r1.remaining);
    REQUIRE(r2.inputs.size() == 1);
    auto* k2 = std::get_if<ParsedKey>(&r2.inputs[0]);
    REQUIRE(k2 != nullptr);
    REQUIRE(k2->name == "b");
    REQUIRE(r2.remaining == "\x1b[C");

    auto r3 = parse_keypress(r2.remaining);
    REQUIRE(r3.inputs.size() == 1);
    auto* k3 = std::get_if<ParsedKey>(&r3.inputs[0]);
    REQUIRE(k3 != nullptr);
    REQUIRE(k3->name == "right");
    REQUIRE(r3.remaining.empty());
}

TEST_CASE("parse_keypress: multiple ESC sequences consumed correctly", "[input]") {
    std::string input = "\x1b[A\x1b[B\x1b[C\x1b[D";

    auto r1 = parse_keypress(input);
    auto* k1 = std::get_if<ParsedKey>(&r1.inputs[0]);
    REQUIRE(k1 != nullptr);
    REQUIRE(k1->name == "up");
    REQUIRE(r1.remaining == "\x1b[B\x1b[C\x1b[D");

    auto r2 = parse_keypress(r1.remaining);
    auto* k2 = std::get_if<ParsedKey>(&r2.inputs[0]);
    REQUIRE(k2 != nullptr);
    REQUIRE(k2->name == "down");
    REQUIRE(r2.remaining == "\x1b[C\x1b[D");

    auto r3 = parse_keypress(r2.remaining);
    auto* k3 = std::get_if<ParsedKey>(&r3.inputs[0]);
    REQUIRE(k3 != nullptr);
    REQUIRE(k3->name == "right");
    REQUIRE(r3.remaining == "\x1b[D");

    auto r4 = parse_keypress(r3.remaining);
    auto* k4 = std::get_if<ParsedKey>(&r4.inputs[0]);
    REQUIRE(k4 != nullptr);
    REQUIRE(k4->name == "left");
    REQUIRE(r4.remaining.empty());
}

TEST_CASE("parse_keypress: SS3 function key followed by remaining text", "[input]") {
    std::string input = "\x1bOPhello";

    auto r1 = parse_keypress(input);
    auto* k1 = std::get_if<ParsedKey>(&r1.inputs[0]);
    REQUIRE(k1 != nullptr);
    REQUIRE(k1->name == "f1");
    REQUIRE(k1->fn);
    REQUIRE(r1.remaining == "hello");

    auto r2 = parse_keypress(r1.remaining);
    auto* k2 = std::get_if<ParsedKey>(&r2.inputs[0]);
    REQUIRE(k2 != nullptr);
    REQUIRE(k2->name == "h");
}

TEST_CASE("parse_keypress: meta key followed by remaining text", "[input]") {
    std::string input = "\033aX";

    auto r1 = parse_keypress(input);
    auto* k1 = std::get_if<ParsedKey>(&r1.inputs[0]);
    REQUIRE(k1 != nullptr);
    REQUIRE(k1->name == "a");
    REQUIRE(k1->meta);
    REQUIRE(r1.remaining == "X");

    auto r2 = parse_keypress(r1.remaining);
    auto* k2 = std::get_if<ParsedKey>(&r2.inputs[0]);
    REQUIRE(k2 != nullptr);
    REQUIRE(k2->name == "x");
    REQUIRE(k2->shift);
}

TEST_CASE("parse_keypress: ctrl key followed by remaining text", "[input]") {
    std::string input = std::string(1, '\x01') + "bc";

    auto r1 = parse_keypress(input);
    auto* k1 = std::get_if<ParsedKey>(&r1.inputs[0]);
    REQUIRE(k1 != nullptr);
    REQUIRE(k1->name == "a");
    REQUIRE(k1->ctrl);
    REQUIRE(r1.remaining == "bc");

    auto r2 = parse_keypress(r1.remaining);
    auto* k2 = std::get_if<ParsedKey>(&r2.inputs[0]);
    REQUIRE(k2 != nullptr);
    REQUIRE(k2->name == "b");
}
