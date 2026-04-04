#pragma once
#include "terminal/color.hpp"
#include <vector>
#include <unordered_map>
#include <string>

namespace ccmake {

class StylePool {
public:
    // Get or create an interned style ID for the given text style
    uint32_t intern(const TextStyle& style);

    // Get the text style for a given ID
    const TextStyle& get(uint32_t id) const;

    // Get the "none" style ID (always 0)
    uint32_t none() const { return 0; }

    // Generate the ANSI escape sequence to transition from one style to another
    // Returns empty string if styles are the same
    std::string transition(uint32_t from_id, uint32_t to_id) const;

    // Number of interned styles
    size_t size() const { return styles_.size(); }

private:
    std::vector<TextStyle> styles_;
    std::unordered_map<size_t, uint32_t> index_;

    static size_t hash_style(const TextStyle& s);
};

}  // namespace ccmake
