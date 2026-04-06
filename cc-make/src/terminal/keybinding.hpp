#pragma once

#include "terminal/input.hpp"

#include <nlohmann/json_fwd.hpp>
#include <string>
#include <unordered_map>
#include <filesystem>

namespace ccmake {

enum class KeyAction {
    None,
    Submit,
    Abort,
    Newline,
    HistoryUp,
    HistoryDown,
    HistorySearch,
    CursorLeft,
    CursorRight,
    CursorHome,
    CursorEnd,
    DeleteBack,
    DeleteForward,
    DeleteWordBack,
    DeleteToStart,
    DeleteToEnd,
    ScrollUp,
    ScrollDown,
    ScrollToTop,
    ScrollToBottom,
    VimEnterNormal,
    VimEnterInsert,
};

struct KeyBinding {
    std::string key_spec;
    KeyAction action;
};

class KeybindingManager {
public:
    KeybindingManager();

    void load_defaults();
    void load_from_config(const std::filesystem::path& config_path);
    void bind(const std::string& key_spec, KeyAction action);
    void unbind(const std::string& key_spec);

    KeyAction resolve(const ParsedKey& key) const;

    nlohmann::json to_json() const;

    bool has_binding(const std::string& key_spec) const;

private:
    std::unordered_map<std::string, KeyAction> bindings_;

    static std::string canonicalize(const ParsedKey& key);
};

}  // namespace ccmake
