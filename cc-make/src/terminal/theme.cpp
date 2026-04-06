#include "terminal/theme.hpp"
#include <nlohmann/json.hpp>

namespace ccmake {

Theme Theme::dark() {
    Theme t;
    t.name = "dark";
    t.colors = {
        Color::rgb(212, 212, 212),    // primary_text (white-ish)
        Color::rgb(128, 128, 128),    // secondary_text (gray)
        Color::rgb(86, 156, 214),     // accent (blue)
        Color::rgb(244, 67, 54),      // error (red)
        Color::rgb(253, 216, 53),     // warning (yellow)
        Color::rgb(76, 175, 80),      // success (green)
        Color::rgb(30, 30, 30),       // background (dark)
        Color::rgb(60, 60, 60),       // border
        Color::rgb(86, 156, 214),     // prompt_marker (blue)
        Color::rgb(156, 163, 175),    // tool_name (gray-blue)
        Color::rgb(128, 128, 128),    // thinking_text (dimmed)
        Color::rgb(212, 212, 212),    // user_message
        Color::rgb(212, 212, 212),    // assistant_message
        Color::rgb(128, 128, 128),    // system_message
        Color::rgb(86, 156, 214),     // tool_result (blue)
        Color::rgb(244, 67, 54),      // tool_error (red)
        Color::rgb(50, 50, 80),       // selection_bg
        Color::rgb(212, 212, 212),    // cursor_color
    };
    return t;
}

Theme Theme::light() {
    Theme t;
    t.name = "light";
    t.colors = {
        Color::rgb(30, 30, 30),       // primary_text (dark)
        Color::rgb(100, 100, 100),    // secondary_text (gray)
        Color::rgb(25, 118, 210),     // accent (blue)
        Color::rgb(211, 47, 47),      // error (red)
        Color::rgb(255, 160, 0),      // warning (orange)
        Color::rgb(46, 125, 50),      // success (green)
        Color::rgb(255, 255, 255),    // background (white)
        Color::rgb(200, 200, 200),    // border
        Color::rgb(25, 118, 210),     // prompt_marker (blue)
        Color::rgb(100, 100, 100),    // tool_name
        Color::rgb(150, 150, 150),    // thinking_text
        Color::rgb(30, 30, 30),       // user_message
        Color::rgb(30, 30, 30),       // assistant_message
        Color::rgb(100, 100, 100),    // system_message
        Color::rgb(25, 118, 210),     // tool_result
        Color::rgb(211, 47, 47),      // tool_error
        Color::rgb(200, 210, 230),    // selection_bg
        Color::rgb(30, 30, 30),       // cursor_color
    };
    return t;
}

Theme Theme::monokai() {
    Theme t;
    t.name = "monokai";
    t.colors = {
        Color::rgb(230, 219, 116),    // primary_text (yellow)
        Color::rgb(100, 100, 100),    // secondary_text
        Color::rgb(102, 217, 239),    // accent (cyan)
        Color::rgb(249, 38, 114),     // error (pink)
        Color::rgb(253, 151, 31),     // warning (orange)
        Color::rgb(166, 226, 46),     // success (green)
        Color::rgb(39, 40, 34),       // background (dark)
        Color::rgb(73, 72, 62),       // border
        Color::rgb(102, 217, 239),    // prompt_marker
        Color::rgb(174, 129, 255),    // tool_name (purple)
        Color::rgb(80, 80, 80),       // thinking_text
        Color::rgb(230, 219, 116),    // user_message
        Color::rgb(248, 248, 242),    // assistant_message (off-white)
        Color::rgb(100, 100, 100),    // system_message
        Color::rgb(102, 217, 239),    // tool_result
        Color::rgb(249, 38, 114),     // tool_error
        Color::rgb(60, 56, 54),       // selection_bg
        Color::rgb(230, 219, 116),    // cursor_color
    };
    return t;
}

nlohmann::json Theme::to_json() const {
    auto color_to_json = [](const Color& c) -> std::string {
        char buf[8];
        snprintf(buf, sizeof(buf), "#%02x%02x%02x", c.r, c.g, c.b);
        return std::string(buf);
    };

    return {
        {"name", name},
        {"use_colors", use_colors},
        {"colors", {
            {"primary_text", color_to_json(colors.primary_text)},
            {"secondary_text", color_to_json(colors.secondary_text)},
            {"accent", color_to_json(colors.accent)},
            {"error", color_to_json(colors.error)},
            {"warning", color_to_json(colors.warning)},
            {"success", color_to_json(colors.success)},
            {"background", color_to_json(colors.background)},
            {"border", color_to_json(colors.border)},
            {"prompt_marker", color_to_json(colors.prompt_marker)},
            {"tool_name", color_to_json(colors.tool_name)},
            {"thinking_text", color_to_json(colors.thinking_text)}
        }}
    };
}

Theme Theme::from_json(const nlohmann::json& j) {
    Theme t;
    t.name = j.value("name", std::string("custom"));

    auto parse_color = [&j](const char* key) -> Color {
        if (j.contains("colors") && j["colors"].contains(key)) {
            return Color::parse(j["colors"][key].get<std::string>());
        }
        return Color::rgb(200, 200, 200);  // fallback
    };

    t.colors.primary_text = parse_color("primary_text");
    t.colors.secondary_text = parse_color("secondary_text");
    t.colors.accent = parse_color("accent");
    t.colors.error = parse_color("error");
    t.colors.warning = parse_color("warning");
    t.colors.success = parse_color("success");
    t.colors.background = parse_color("background");
    t.colors.border = parse_color("border");
    t.colors.prompt_marker = parse_color("prompt_marker");
    t.colors.tool_name = parse_color("tool_name");
    t.colors.thinking_text = parse_color("thinking_text");

    return t;
}

ThemeManager::ThemeManager(const std::filesystem::path& config_dir)
    : config_dir_(config_dir) {
    register_builtins();
}

void ThemeManager::register_builtins() {
    builtins_["dark"] = Theme::dark();
    builtins_["light"] = Theme::light();
    builtins_["monokai"] = Theme::monokai();
    current_ = builtins_["dark"];
}

const Theme& ThemeManager::current() const {
    return current_;
}

void ThemeManager::set_theme(const std::string& name) {
    auto it = builtins_.find(name);
    if (it != builtins_.end()) {
        current_ = it->second;
    }
}

std::vector<std::string> ThemeManager::available_themes() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : builtins_) {
        names.push_back(name);
    }
    return names;
}

Color ThemeManager::color_for(const std::string& role) const {
    if (role == "primary_text") return current_.colors.primary_text;
    if (role == "secondary_text") return current_.colors.secondary_text;
    if (role == "accent") return current_.colors.accent;
    if (role == "error") return current_.colors.error;
    if (role == "warning") return current_.colors.warning;
    if (role == "success") return current_.colors.success;
    if (role == "background") return current_.colors.background;
    if (role == "border") return current_.colors.border;
    if (role == "prompt_marker") return current_.colors.prompt_marker;
    if (role == "tool_name") return current_.colors.tool_name;
    if (role == "thinking_text") return current_.colors.thinking_text;
    if (role == "user_message") return current_.colors.user_message;
    if (role == "assistant_message") return current_.colors.assistant_message;
    if (role == "system_message") return current_.colors.system_message;
    if (role == "tool_result") return current_.colors.tool_result;
    if (role == "tool_error") return current_.colors.tool_error;
    if (role == "selection_bg") return current_.colors.selection_bg;
    if (role == "cursor_color") return current_.colors.cursor_color;
    return current_.colors.primary_text;
}

}  // namespace ccmake
