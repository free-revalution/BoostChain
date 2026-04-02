#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace ccmake {

/// Remove leading and trailing whitespace from a string.
inline std::string trim(std::string s) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
    return s;
}

/// Split a string by a single-character delimiter.
inline std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    size_t start = 0;
    size_t pos = s.find(delim);
    while (pos != std::string::npos) {
        parts.push_back(s.substr(start, pos - start));
        start = pos + 1;
        pos = s.find(delim, start);
    }
    parts.push_back(s.substr(start));
    return parts;
}

/// Split a string by a multi-character delimiter.
inline std::vector<std::string> split(const std::string& s, const std::string& delim) {
    std::vector<std::string> parts;
    if (delim.empty()) {
        parts.push_back(s);
        return parts;
    }
    size_t start = 0;
    size_t pos = s.find(delim);
    while (pos != std::string::npos) {
        parts.push_back(s.substr(start, pos - start));
        start = pos + delim.size();
        pos = s.find(delim, start);
    }
    parts.push_back(s.substr(start));
    return parts;
}

/// Check if a string starts with the given prefix.
inline bool starts_with(const std::string& s, const std::string& prefix) {
    if (prefix.size() > s.size()) return false;
    return s.compare(0, prefix.size(), prefix) == 0;
}

/// Check if a string ends with the given suffix.
inline bool ends_with(const std::string& s, const std::string& suffix) {
    if (suffix.size() > s.size()) return false;
    return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

/// Check if a string contains a substring.
inline bool contains(const std::string& s, const std::string& substr) {
    return s.find(substr) != std::string::npos;
}

/// Convert a string to lowercase.
inline std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}

/// Convert a string to uppercase.
inline std::string to_upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
    return s;
}

/// Join a vector of strings with a separator.
inline std::string join(const std::vector<std::string>& parts, const std::string& sep) {
    if (parts.empty()) return "";
    std::string result = parts[0];
    for (size_t i = 1; i < parts.size(); ++i) {
        result += sep;
        result += parts[i];
    }
    return result;
}

/// Replace all occurrences of `from` with `to` in a string.
inline std::string replace_all(std::string s, const std::string& from, const std::string& to) {
    if (from.empty()) return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

}  // namespace ccmake
