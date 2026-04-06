#include "terminal/keybinding.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <algorithm>

namespace ccmake {

KeybindingManager::KeybindingManager() {
    load_defaults();
}

void KeybindingManager::load_defaults() {
    bindings_ = {
        {"return", KeyAction::Submit},
        {"enter", KeyAction::Submit},
        {"escape", KeyAction::Abort},
        {"ctrl+c", KeyAction::Abort},
        {"ctrl+d", KeyAction::Abort},
        {"up", KeyAction::HistoryUp},
        {"down", KeyAction::HistoryDown},
        {"ctrl+p", KeyAction::HistoryUp},
        {"ctrl+n", KeyAction::HistoryDown},
        {"left", KeyAction::CursorLeft},
        {"right", KeyAction::CursorRight},
        {"ctrl+a", KeyAction::CursorHome},
        {"ctrl+e", KeyAction::CursorEnd},
        {"ctrl+b", KeyAction::CursorLeft},
        {"ctrl+f", KeyAction::CursorRight},
        {"backspace", KeyAction::DeleteBack},
        {"ctrl+h", KeyAction::DeleteBack},
        {"delete", KeyAction::DeleteForward},
        {"ctrl+w", KeyAction::DeleteWordBack},
        {"ctrl+u", KeyAction::DeleteToStart},
        {"ctrl+k", KeyAction::DeleteToEnd},
        {"ctrl+y", KeyAction::Newline},
        {"pageup", KeyAction::ScrollUp},
        {"pagedown", KeyAction::ScrollDown},
        {"home", KeyAction::ScrollToTop},
        {"end", KeyAction::ScrollToBottom},
    };
}

void KeybindingManager::load_from_config(const std::filesystem::path& config_path) {
    if (!std::filesystem::exists(config_path)) return;

    try {
        YAML::Node root = YAML::LoadFile(config_path.string());
        if (root["keybindings"] && root["keybindings"].IsMap()) {
            for (const auto& entry : root["keybindings"]) {
                std::string key_spec = entry.first.as<std::string>();
                std::string action_str = entry.second.as<std::string>();

                KeyAction action = KeyAction::None;
                if (action_str == "submit") action = KeyAction::Submit;
                else if (action_str == "abort") action = KeyAction::Abort;
                else if (action_str == "newline") action = KeyAction::Newline;
                else if (action_str == "history_up") action = KeyAction::HistoryUp;
                else if (action_str == "history_down") action = KeyAction::HistoryDown;
                else if (action_str == "cursor_left") action = KeyAction::CursorLeft;
                else if (action_str == "cursor_right") action = KeyAction::CursorRight;
                else if (action_str == "cursor_home") action = KeyAction::CursorHome;
                else if (action_str == "cursor_end") action = KeyAction::CursorEnd;
                else if (action_str == "delete_back") action = KeyAction::DeleteBack;
                else if (action_str == "delete_forward") action = KeyAction::DeleteForward;
                else if (action_str == "delete_word_back") action = KeyAction::DeleteWordBack;
                else if (action_str == "delete_to_start") action = KeyAction::DeleteToStart;
                else if (action_str == "delete_to_end") action = KeyAction::DeleteToEnd;
                else if (action_str == "scroll_up") action = KeyAction::ScrollUp;
                else if (action_str == "scroll_down") action = KeyAction::ScrollDown;

                if (action != KeyAction::None) {
                    bindings_[key_spec] = action;
                }
            }
        }
    } catch (const std::exception& e) {
        spdlog::warn("Failed to load keybindings: {}", e.what());
    }
}

void KeybindingManager::bind(const std::string& key_spec, KeyAction action) {
    bindings_[key_spec] = action;
}

void KeybindingManager::unbind(const std::string& key_spec) {
    bindings_.erase(key_spec);
}

std::string KeybindingManager::canonicalize(const ParsedKey& key) {
    std::string spec;

    if (key.ctrl) spec += "ctrl+";
    if (key.meta) spec += "alt+";
    if (key.shift) spec += "shift+";

    std::string name = key.name;
    if (name == "return") name = "enter";

    spec += name;
    return spec;
}

KeyAction KeybindingManager::resolve(const ParsedKey& key) const {
    auto spec = canonicalize(key);
    if (spec.empty()) return KeyAction::None;

    auto it = bindings_.find(spec);
    if (it != bindings_.end()) {
        return it->second;
    }
    return KeyAction::None;
}

nlohmann::json KeybindingManager::to_json() const {
    auto action_to_string = [](KeyAction a) -> std::string {
        switch (a) {
            case KeyAction::Submit: return "submit";
            case KeyAction::Abort: return "abort";
            case KeyAction::Newline: return "newline";
            case KeyAction::HistoryUp: return "history_up";
            case KeyAction::HistoryDown: return "history_down";
            case KeyAction::CursorLeft: return "cursor_left";
            case KeyAction::CursorRight: return "cursor_right";
            case KeyAction::CursorHome: return "cursor_home";
            case KeyAction::CursorEnd: return "cursor_end";
            case KeyAction::DeleteBack: return "delete_back";
            case KeyAction::DeleteForward: return "delete_forward";
            case KeyAction::DeleteWordBack: return "delete_word_back";
            case KeyAction::DeleteToStart: return "delete_to_start";
            case KeyAction::DeleteToEnd: return "delete_to_end";
            case KeyAction::ScrollUp: return "scroll_up";
            case KeyAction::ScrollDown: return "scroll_down";
            case KeyAction::ScrollToTop: return "scroll_to_top";
            case KeyAction::ScrollToBottom: return "scroll_to_bottom";
            default: return "none";
        }
    };

    nlohmann::json j = nlohmann::json::object();
    for (const auto& [spec, action] : bindings_) {
        j[spec] = action_to_string(action);
    }
    return j;
}

bool KeybindingManager::has_binding(const std::string& key_spec) const {
    return bindings_.count(key_spec) > 0;
}

}  // namespace ccmake
