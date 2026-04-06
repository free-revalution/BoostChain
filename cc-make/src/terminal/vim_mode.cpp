#include "terminal/vim_mode.hpp"

#include <cctype>

namespace ccmake {

VimModeHandler::VimModeHandler() = default;

VimMode VimModeHandler::mode() const {
    return mode_;
}

void VimModeHandler::enter_insert() {
    mode_ = VimMode::Insert;
    pending_operator_.clear();
    pending_motion_.clear();
    count_.reset();
    count_str_.clear();
}

void VimModeHandler::enter_normal() {
    mode_ = VimMode::Normal;
    pending_operator_.clear();
    pending_motion_.clear();
    count_.reset();
    count_str_.clear();
}

std::optional<KeyAction> VimModeHandler::process_key(const ParsedKey& key) {
    if (mode_ == VimMode::Insert) {
        return handle_insert(key);
    } else {
        return handle_normal(key);
    }
}

std::optional<int> VimModeHandler::parse_count(const ParsedKey& key) {
    if (key.name.size() == 1 && std::isdigit(key.name[0])) {
        count_str_ += key.name;
        return std::nullopt;
    }
    if (!count_str_.empty()) {
        int count = std::stoi(count_str_);
        count_str_.clear();
        count_ = count;
    }
    return std::nullopt;
}

std::optional<KeyAction> VimModeHandler::handle_normal(const ParsedKey& key) {
    std::string c = key.name;

    // Parse count prefix (0 is a motion, not a count digit)
    if (c.size() == 1 && std::isdigit(c[0]) && c != "0") {
        parse_count(key);
        return std::nullopt;
    }

    // Motion keys
    if (key.name == "up" || c == "k") return KeyAction::CursorLeft;
    if (key.name == "down" || c == "j") return KeyAction::CursorRight;
    if (key.name == "left" || c == "h") return KeyAction::CursorLeft;
    if (key.name == "right" || c == "l") return KeyAction::CursorRight;

    // Enter insert mode
    if (c == "i") { enter_insert(); return std::nullopt; }
    if (c == "a") { enter_insert(); return KeyAction::CursorRight; }
    if (c == "A") { enter_insert(); return KeyAction::CursorEnd; }
    if (c == "I") { enter_insert(); return KeyAction::CursorHome; }
    if (c == "o") { enter_insert(); return KeyAction::Newline; }

    // Operators
    if (c == "d" || c == "c" || c == "y") {
        if (!pending_operator_.empty()) {
            pending_motion_ = c;
            auto result = resolve_operator_motion();
            pending_operator_.clear();
            pending_motion_.clear();
            return result;
        }
        pending_operator_ = c;
        return std::nullopt;
    }

    // Motions (no operator pending)
    if (pending_operator_.empty()) {
        if (c == "w") return KeyAction::CursorRight;
        if (c == "b") return KeyAction::CursorLeft;
        if (c == "0") return KeyAction::CursorHome;
        if (c == "$") return KeyAction::CursorEnd;
        if (c == "G") return KeyAction::ScrollToBottom;
        if (pending_motion_ == "g" && c == "g") {
            pending_motion_.clear();
            return KeyAction::ScrollToTop;
        }
        if (c == "g") {
            pending_motion_ = "g";
            return std::nullopt;
        }
        if (c == "p") return KeyAction::Newline;
        if (c == "u") return KeyAction::Newline;
        return std::nullopt;
    }

    // Motion after operator
    pending_motion_ = c;
    auto result = resolve_operator_motion();
    pending_operator_.clear();
    pending_motion_.clear();
    return result;
}

std::optional<KeyAction> VimModeHandler::handle_insert(const ParsedKey& key) {
    if (key.name == "escape") {
        enter_normal();
        return KeyAction::CursorLeft;
    }

    if (key.ctrl && key.name == "c") {
        enter_normal();
        return KeyAction::Abort;
    }

    return std::nullopt;
}

std::optional<KeyAction> VimModeHandler::resolve_operator_motion() {
    if (pending_operator_.empty() || pending_motion_.empty()) {
        return std::nullopt;
    }

    char op = pending_operator_[0];
    char motion = pending_motion_[0];

    if (op == motion) {
        if (op == 'c') {
            enter_insert();
            return KeyAction::DeleteToStart;
        }
        return KeyAction::DeleteToStart;
    }

    if (motion == 'w' || motion == 'b') {
        return KeyAction::DeleteWordBack;
    }

    return std::nullopt;
}

}  // namespace ccmake
