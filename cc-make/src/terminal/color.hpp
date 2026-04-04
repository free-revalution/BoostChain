#pragma once
#include <string>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <algorithm>
#include <cctype>

namespace ccmake {

// Forward declaration
std::optional<uint8_t> parse_ansi_color_name(const std::string& name);

enum class ColorType { RGB, Ansi256, Ansi16, Hex };

struct Color {
    ColorType type;
    uint8_t r = 0, g = 0, b = 0;  // RGB values (also used for Ansi16 index)

    // Factory methods
    static Color rgb(uint8_t r, uint8_t g, uint8_t b) {
        return {ColorType::RGB, r, g, b};
    }
    static Color ansi256(uint8_t index) {
        return {ColorType::Ansi256, index, 0, 0};
    }
    static Color ansi16(uint8_t index) {
        return {ColorType::Ansi16, index, 0, 0};
    }
    static Color hex(const std::string& hex) {
        std::string h = hex;
        // Strip leading '#'
        if (!h.empty() && h[0] == '#') h = h.substr(1);
        // Support shorthand 3-char hex (#RGB -> #RRGGBB)
        if (h.size() == 3) {
            h = std::string(2, h[0]) + std::string(2, h[1]) + std::string(2, h[2]);
        }
        uint8_t rv = 0, gv = 0, bv = 0;
        if (h.size() >= 2) rv = static_cast<uint8_t>(std::stoi(h.substr(0, 2), nullptr, 16));
        if (h.size() >= 4) gv = static_cast<uint8_t>(std::stoi(h.substr(2, 2), nullptr, 16));
        if (h.size() >= 6) bv = static_cast<uint8_t>(std::stoi(h.substr(4, 2), nullptr, 16));
        return {ColorType::Hex, rv, gv, bv};
    }

    static Color parse(const std::string& str) {
        auto s = str;
        // Trim whitespace
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s = s.substr(1);
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();

        if (s.rfind("rgb(", 0) == 0 && s.back() == ')') {
            // rgb(R,G,B)
            auto inner = s.substr(4, s.size() - 5);
            uint8_t rv = 0, gv = 0, bv = 0;
            size_t pos = 0;
            auto next = inner.find(',');
            if (next != std::string::npos) {
                rv = static_cast<uint8_t>(std::stoi(inner.substr(pos, next)));
                pos = next + 1;
                next = inner.find(',', pos);
                if (next != std::string::npos) {
                    gv = static_cast<uint8_t>(std::stoi(inner.substr(pos, next)));
                    pos = next + 1;
                    bv = static_cast<uint8_t>(std::stoi(inner.substr(pos)));
                }
            }
            return rgb(rv, gv, bv);
        }
        if (s.rfind("ansi256(", 0) == 0 && s.back() == ')') {
            auto inner = s.substr(8, s.size() - 9);
            auto idx = static_cast<uint8_t>(std::stoi(inner));
            return ansi256(idx);
        }
        if (s.rfind("ansi:", 0) == 0) {
            auto name = s.substr(5);
            auto idx = parse_ansi_color_name(name);
            if (idx) return ansi16(*idx);
            return rgb(0, 0, 0); // fallback
        }
        if (!s.empty() && s[0] == '#') {
            return hex(s);
        }
        // Fallback
        return rgb(0, 0, 0);
    }

    // Convert to ANSI SGR parameter string
    std::string to_sgr_fg() const {
        switch (type) {
            case ColorType::RGB:
            case ColorType::Hex:
                return "38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b);
            case ColorType::Ansi256:
                return "38;5;" + std::to_string(r);
            case ColorType::Ansi16:
                return std::to_string(30 + r);
        }
        return "39";
    }

    std::string to_sgr_bg() const {
        switch (type) {
            case ColorType::RGB:
            case ColorType::Hex:
                return "48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b);
            case ColorType::Ansi256:
                return "48;5;" + std::to_string(r);
            case ColorType::Ansi16:
                return std::to_string(40 + r);
        }
        return "49";
    }

    bool operator==(const Color& o) const { return type == o.type && r == o.r && g == o.g && b == o.b; }
};

// Text style properties
struct TextStyle {
    std::optional<Color> color;
    std::optional<Color> bg_color;
    bool bold = false;
    bool dim = false;
    bool italic = false;
    bool underline = false;
    bool strikethrough = false;
    bool inverse = false;

    bool operator==(const TextStyle& o) const = default;
    bool is_plain() const { return !color && !bg_color && !bold && !dim && !italic && !underline && !strikethrough && !inverse; }
};

// Named ANSI color parsing
inline std::optional<uint8_t> parse_ansi_color_name(const std::string& name) {
    static const std::unordered_map<std::string, uint8_t> names = {
        {"black", 0}, {"red", 1}, {"green", 2}, {"yellow", 3},
        {"blue", 4}, {"magenta", 5}, {"cyan", 6}, {"white", 7},
        {"blackBright", 8}, {"redBright", 9}, {"greenBright", 10}, {"yellowBright", 11},
        {"blueBright", 12}, {"magentaBright", 13}, {"cyanBright", 14}, {"whiteBright", 15},
        // Also accept "bright_xxx" form
        {"brightBlack", 8}, {"brightRed", 9}, {"brightGreen", 10}, {"brightYellow", 11},
        {"brightBlue", 12}, {"brightMagenta", 13}, {"brightCyan", 14}, {"brightWhite", 15},
    };
    auto it = names.find(name);
    if (it != names.end()) return it->second;
    return std::nullopt;
}

}  // namespace ccmake
