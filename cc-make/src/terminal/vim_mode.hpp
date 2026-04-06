#pragma once

#include "terminal/input.hpp"
#include "terminal/keybinding.hpp"

#include <optional>
#include <string>

namespace ccmake {

enum class VimMode { Insert, Normal };

class VimModeHandler {
public:
    VimModeHandler();

    std::optional<KeyAction> process_key(const ParsedKey& key);

    VimMode mode() const;

    void enter_insert();
    void enter_normal();

private:
    VimMode mode_ = VimMode::Insert;
    std::string pending_operator_;
    std::string pending_motion_;
    std::optional<int> count_;
    std::string count_str_;

    std::optional<KeyAction> handle_normal(const ParsedKey& key);
    std::optional<KeyAction> handle_insert(const ParsedKey& key);
    std::optional<KeyAction> resolve_operator_motion();

    std::optional<int> parse_count(const ParsedKey& key);
    std::string key_to_char(const ParsedKey& key) const;
};

}  // namespace ccmake
