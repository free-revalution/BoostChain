#include "terminal/style_pool.hpp"

namespace ccmake {

size_t StylePool::hash_style(const TextStyle& s) {
    size_t h = 0u;
    // Hash boolean flags
    auto hash_bool = [&](bool v) {
        h = h * 31 + (v ? 1u : 0u);
    };
    hash_bool(s.bold);
    hash_bool(s.dim);
    hash_bool(s.italic);
    hash_bool(s.underline);
    hash_bool(s.strikethrough);
    hash_bool(s.inverse);
    // Hash color (foreground)
    if (s.color) {
        h = h * 31 + 1; // marker: has color
        h = h * 31 + static_cast<size_t>(s.color->type);
        h = h * 31 + s.color->r;
        h = h * 31 + s.color->g;
        h = h * 31 + s.color->b;
    } else {
        h = h * 31 + 0; // marker: no color
    }
    // Hash bg_color
    if (s.bg_color) {
        h = h * 31 + 1; // marker: has bg_color
        h = h * 31 + static_cast<size_t>(s.bg_color->type);
        h = h * 31 + s.bg_color->r;
        h = h * 31 + s.bg_color->g;
        h = h * 31 + s.bg_color->b;
    } else {
        h = h * 31 + 0; // marker: no bg_color
    }
    return h;
}

uint32_t StylePool::intern(const TextStyle& style) {
    size_t h = hash_style(style);
    auto it = index_.find(h);
    if (it != index_.end()) {
        // Verify it's actually the same style (hash collision guard)
        if (styles_[it->second] == style) {
            return it->second;
        }
    }
    auto id = static_cast<uint32_t>(styles_.size());
    styles_.push_back(style);
    index_[h] = id;
    return id;
}

const TextStyle& StylePool::get(uint32_t id) const {
    return styles_.at(id);
}

std::string StylePool::transition(uint32_t from_id, uint32_t to_id) const {
    if (from_id == to_id) return "";

    const TextStyle& from = styles_.at(from_id);
    const TextStyle& to = styles_.at(to_id);

    // Collect SGR codes
    std::vector<std::string> codes;

    // Foreground color
    if (from.color != to.color) {
        if (to.color) {
            codes.push_back(to.color->to_sgr_fg());
        } else if (from.color) {
            codes.push_back("39");
        }
    }

    // Background color
    if (from.bg_color != to.bg_color) {
        if (to.bg_color) {
            codes.push_back(to.bg_color->to_sgr_bg());
        } else if (from.bg_color) {
            codes.push_back("49");
        }
    }

    // Boolean attributes
    if (from.bold != to.bold) codes.push_back(to.bold ? "1" : "22");
    if (from.dim != to.dim) codes.push_back(to.dim ? "2" : "22");
    if (from.italic != to.italic) codes.push_back(to.italic ? "3" : "23");
    if (from.underline != to.underline) codes.push_back(to.underline ? "4" : "24");
    if (from.strikethrough != to.strikethrough) codes.push_back(to.strikethrough ? "9" : "29");
    if (from.inverse != to.inverse) codes.push_back(to.inverse ? "7" : "27");

    if (codes.empty()) return "";

    // Build the SGR escape sequence
    std::string result = "\x1b[";
    for (size_t i = 0; i < codes.size(); ++i) {
        if (i > 0) result += ";";
        result += codes[i];
    }
    result += "m";
    return result;
}

}  // namespace ccmake
