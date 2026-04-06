#pragma once

#include "terminal/input.hpp"

#include <string>
#include <vector>

namespace ccmake {

struct InputLine {
    std::string text;
    int cursor_pos = 0;
};

class PromptInput {
public:
    PromptInput();

    bool handle_key(const ParsedKey& key);
    std::string text() const;
    bool is_multiline() const;
    const std::vector<InputLine>& lines() const;
    std::string render(int terminal_width) const;

    // History
    void push_history(const std::string& line);
    void history_up();
    void history_down();

    // Multiline
    void enter_multiline();
    void exit_multiline();
    void newline();

    // Kill ring
    void kill_to_start();
    void kill_to_end();
    void yank();

    void clear();

    int cursor_row() const;
    int cursor_col() const;

private:
    std::vector<InputLine> lines_;
    int current_line_ = 0;
    bool multiline_ = false;
    std::vector<std::string> history_;
    int history_index_ = -1;
    std::string saved_input_;
    std::string kill_ring_;

    void insert_char(char c);
    void delete_char_back();
    void delete_char_forward();
    void move_cursor(int delta);
    void move_cursor_to(int pos);

    InputLine& current_line();
    const InputLine& current_line() const;
};

}  // namespace ccmake
