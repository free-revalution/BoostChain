#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <optional>

#include "core/result.hpp"

namespace ccmake {

/// Get a value from a JSON object, returning a default if the key is missing
/// or the type does not match.
template <typename T>
T json_get_or(const nlohmann::json& j, const std::string& key, T default_val) {
    auto it = j.find(key);
    if (it == j.end()) return default_val;
    try {
        return it->get<T>();
    } catch (...) {
        return default_val;
    }
}

/// Specialization for std::string: handles non-string JSON types by returning
/// the default instead of throwing.
template <>
inline std::string json_get_or<std::string>(const nlohmann::json& j,
                                             const std::string& key,
                                             std::string default_val) {
    auto it = j.find(key);
    if (it == j.end()) return default_val;
    if (!it->is_string()) return default_val;
    return it->get<std::string>();
}

/// Parse a JSON string without throwing. Returns Ok(json) or Err(error_message).
inline Result<nlohmann::json, std::string> json_safe_parse(std::string_view str) {
    try {
        return Result<nlohmann::json, std::string>::ok(nlohmann::json::parse(str));
    } catch (const nlohmann::json::parse_error& e) {
        return Result<nlohmann::json, std::string>::err(e.what());
    }
}

/// Get a string from a JSON object. Returns nullopt if missing or not a string.
inline std::optional<std::string> json_get_string(const nlohmann::json& j,
                                                    const std::string& key) {
    auto it = j.find(key);
    if (it == j.end() || !it->is_string()) return std::nullopt;
    return it->get<std::string>();
}

/// Get an int64_t from a JSON object. Returns nullopt if missing or not a number.
inline std::optional<int64_t> json_get_int(const nlohmann::json& j,
                                             const std::string& key) {
    auto it = j.find(key);
    if (it == j.end() || !it->is_number_integer()) return std::nullopt;
    return it->get<int64_t>();
}

/// Get a bool from a JSON object. Returns nullopt if missing or not a boolean.
inline std::optional<bool> json_get_bool(const nlohmann::json& j,
                                          const std::string& key) {
    auto it = j.find(key);
    if (it == j.end() || !it->is_boolean()) return std::nullopt;
    return it->get<bool>();
}

/// Safely serialize a JSON value to a string. Uses no indentation by default.
/// Passing indent = -1 means compact (no whitespace).
inline std::string json_to_string(const nlohmann::json& j, int indent = -1) {
    return j.dump(indent);
}

}  // namespace ccmake
