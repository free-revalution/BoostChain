#include "ui/prompt_input.hpp"

#include <algorithm>

namespace ccmake {

PromptInput::PromptInput() {
    lines_.push_back(InputLine{});
}

InputLine& PromptInput::current_line() {
    return lines_[current_line_];
}

const InputLine& PromptInput::current_line() const {
    return lines_[current_line_];
}

bool PromptInput::handle_key(const ParsedKey& key) {
    std::string name = key.name;

    if (name == "return" || name == "enter") {
        if (multiline_) {
            newline();
        } else {
            return false;  // Let caller handle submit
        }
        return true;
    }

    if (name == "backspace") {
        delete_char_back();
        return true;
    }

    if (name == "delete") {
        delete_char_forward();
        return true;
    }

    if (name == "escape") {
        return false;
    }

    // Ctrl+A: home
    if (key.ctrl && (name == "a")) {
        move_cursor_to(0);
        return true;
    }

    // Ctrl+E: end
    if (key.ctrl && (name == "e")) {
        move_cursor_to(static_cast<int>(current_line().text.size()));
        return true;
    }

    // Ctrl+W: delete word back
    if (key.ctrl && (name == "w")) {
        kill_to_start();
        return true;
    }

    // Ctrl+U: delete to start
    if (key.ctrl && (name == "u")) {
        kill_to_start();
        return true;
    }

    // Ctrl+K: delete to end
    if (key.ctrl && (name == "k")) {
        kill_to_end();
        return true;
    }

    // Ctrl+Y: yank
    if (key.ctrl && (name == "y")) {
        yank();
        return true;
    }

    // Ctrl+D: abort on empty line
    if (key.ctrl && (name == "d")) {
        if (current_line().text.empty()) {
            return false;  // Let caller handle EOF
        }
    }

    // Left/Right arrow
    if (name == "left") {
        move_cursor(-1);
        return true;
    }
    if (name == "right") {
        move_cursor(1);
        return true;
    }
    if (name == "home") {
        move_cursor_to(0);
        return true;
    }
    if (name == "end") {
        move_cursor_to(static_cast<int>(current_line().text.size()));
        return true;
    }

    // Regular character input (single character name like 'a', 'b', etc.)
    if (name.size() == 1 && !key.ctrl && !key.meta) {
        insert_char(name[0]);
        return true;
    }

    return false;
}

void PromptInput::insert_char(char c) {
    auto& line = current_line();
    line.text.insert(line.cursor_pos, 1, c);
    line.cursor_pos++;
}

void PromptInput::delete_char_back() {
    auto& line = current_line();
    if (line.cursor_pos > 0) {
        line.text.erase(line.cursor_pos - 1, 1);
        line.cursor_pos--;
    }
}

void PromptInput::delete_char_forward() {
    auto& line = current_line();
    if (line.cursor_pos < static_cast<int>(line.text.size())) {
        line.text.erase(line.cursor_pos, 1);
    }
}

void PromptInput::move_cursor(int delta) {
    auto& line = current_line();
    line.cursor_pos += delta;
    if (line.cursor_pos < 0) line.cursor_pos = 0;
    int max_pos = static_cast<int>(line.text.size());
    if (line.cursor_pos > max_pos) line.cursor_pos = max_pos;
}

void PromptInput::move_cursor_to(int pos) {
    auto& line = current_line();
    if (pos < 0) pos = 0;
    int max_pos = static_cast<int>(line.text.size());
    if (pos > max_pos) pos = max_pos;
    line.cursor_pos = pos;
}

void PromptInput::push_history(const std::string& line) {
    if (!line.empty()) {
        history_.push_back(line);
        history_index_ = static_cast<int>(history_.size());
    }
}

void PromptInput::history_up() {
    if (history_.empty()) return;
    if (history_index_ > 0) {
        if (history_index_ == static_cast<int>(history_.size())) {
            saved_input_ = current_line().text;
        }
        history_index_--;
        current_line().text = history_[history_index_];
        current_line().cursor_pos = static_cast<int>(current_line().text.size());
    }
}

void PromptInput::history_down() {
    if (history_index_ < static_cast<int>(history_.size()) - 1) {
        history_index_++;
        current_line().text = history_[history_index_];
        current_line().cursor_pos = static_cast<int>(current_line().text.size());
    } else if (history_index_ == static_cast<int>(history_.size()) - 1) {
        history_index_++;
        current_line().text = saved_input_;
        current_line().cursor_pos = static_cast<int>(current_line().text.size());
        saved_input_.clear();
    }
}

void PromptInput::enter_multiline() {
    multiline_ = true;
}

void PromptInput::exit_multiline() {
    multiline_ = false;
    if (lines_.size() > 1) {
        lines_.resize(1);
        current_line_ = 0;
    }
}

void PromptInput::newline() {
    lines_.push_back(InputLine{});
    current_line_ = static_cast<int>(lines_.size()) - 1;
}

void PromptInput::kill_to_start() {
    auto& line = current_line();
    if (line.cursor_pos > 0) {
        kill_ring_ = line.text.substr(0, line.cursor_pos);
        line.text = line.text.substr(line.cursor_pos);
        line.cursor_pos = 0;
    }
}

void PromptInput::kill_to_end() {
    auto& line = current_line();
    if (line.cursor_pos < static_cast<int>(line.text.size())) {
        kill_ring_ = line.text.substr(line.cursor_pos);
        line.text = line.text.substr(0, line.cursor_pos);
    }
}

void PromptInput::yank() {
    if (!kill_ring_.empty()) {
        auto& line = current_line();
        line.text.insert(line.cursor_pos, kill_ring_);
        line.cursor_pos += static_cast<int>(kill_ring_.size());
    }
}

void PromptInput::clear() {
    lines_.clear();
    lines_.push_back(InputLine{});
    current_line_ = 0;
}

std::string PromptInput::text() const {
    if (lines_.size() == 1) {
        return lines_[0].text;
    }
    std::string result;
    for (size_t i = 0; i < lines_.size(); ++i) {
        if (i > 0) result += "\n";
        result += lines_[i].text;
    }
    return result;
}

bool PromptInput::is_multiline() const {
    return multiline_;
}

const std::vector<InputLine>& PromptInput::lines() const {
    return lines_;
}

std::string PromptInput::render(int terminal_width) const {
    (void)terminal_width;
    if (lines_.size() == 1) {
        return "> " + lines_[0].text;
    }
    std::string result;
    for (size_t i = 0; i < lines_.size(); ++i) {
        if (i > 0) result += "\n";
        result += lines_[i].text;
    }
    return result;
}

int PromptInput::cursor_row() const {
    return current_line_;
}

int PromptInput::cursor_col() const {
    return current_line().cursor_pos;
}

}  // namespace ccmake
