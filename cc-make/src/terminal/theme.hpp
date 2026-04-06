#pragma once

#include "terminal/color.hpp"

#include <nlohmann/json_fwd.hpp>
#include <string>
#include <unordered_map>
#include <filesystem>

namespace ccmake {

struct ThemeColors {
    Color primary_text;
    Color secondary_text;
    Color accent;
    Color error;
    Color warning;
    Color success;
    Color background;
    Color border;
    Color prompt_marker;
    Color tool_name;
    Color thinking_text;
    Color user_message;
    Color assistant_message;
    Color system_message;
    Color tool_result;
    Color tool_error;
    Color selection_bg;
    Color cursor_color;
};

struct Theme {
    std::string name;
    ThemeColors colors;
    bool use_colors = true;

    nlohmann::json to_json() const;
    static Theme from_json(const nlohmann::json& j);

    static Theme dark();
    static Theme light();
    static Theme monokai();
};

class ThemeManager {
public:
    explicit ThemeManager(const std::filesystem::path& config_dir);

    const Theme& current() const;
    void set_theme(const std::string& name);
    std::vector<std::string> available_themes() const;

    // Get a Color for a semantic role
    Color color_for(const std::string& role) const;

private:
    Theme current_;
    std::unordered_map<std::string, Theme> builtins_;
    std::filesystem::path config_dir_;

    void register_builtins();
};

}  // namespace ccmake
