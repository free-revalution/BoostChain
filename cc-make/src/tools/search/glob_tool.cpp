#include "tools/search/glob_tool.hpp"
#include "core/error.hpp"

#include <filesystem>
#include <regex>
#include <algorithm>
#include <sstream>

namespace ccmake {

GlobTool::GlobTool() = default;

const ToolDefinition& GlobTool::definition() const {
    static ToolDefinition def = [] {
        ToolDefinition d;
        d.name = "Glob";
        d.description = "Find files matching a glob pattern in a directory.";
        d.input_schema = {
            {"type", "object"},
            {"properties", {
                {"pattern", {{"type", "string"}, {"description", "The glob pattern to match files"}}},
                {"path", {{"type", "string"}, {"description", "The directory to search in (default: cwd)"}}}
            }},
            {"required", {"pattern"}}
        };
        d.aliases = {"FileGlob"};
        d.is_read_only = true;
        d.is_concurrency_safe = true;
        return d;
    }();
    return def;
}

std::string GlobTool::validate_input(const nlohmann::json& input, const ToolContext& ctx) const {
    if (!input.contains("pattern") || !input["pattern"].is_string()) {
        return "pattern is required and must be a string";
    }
    if (input["pattern"].get<std::string>().empty()) {
        return "pattern must not be empty";
    }
    return "";
}

// Split a path into segments by '/'
static std::vector<std::string> split_path(const std::string& path) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : path) {
        if (c == '/') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) parts.push_back(current);
    return parts;
}

// Convert a single glob segment (e.g., "*.js") to a regex string
static std::string glob_segment_to_regex(const std::string& seg) {
    std::string regex;
    for (size_t i = 0; i < seg.size(); ++i) {
        char c = seg[i];
        switch (c) {
            case '*': regex += ".*"; break;
            case '?': regex += "."; break;
            case '.': case '+': case '^': case '$': case '|': case '(':
            case ')': case '[': case ']': case '{': case '}': case '\\':
                regex += '\\'; regex += c; break;
            default: regex += c; break;
        }
    }
    return regex;
}

// Match segments[text_start..] against segments[pat_start..]
// Returns true if the remaining text segments match the remaining pattern segments
static bool match_segments(const std::vector<std::string>& text_segs, size_t ti,
                           const std::vector<std::string>& pat_segs, size_t pi) {
    while (pi < pat_segs.size()) {
        if (pat_segs[pi] == "**") {
            // ** matches zero or more path segments
            ++pi;
            // Try consuming 0, 1, 2, ... text segments with **
            for (size_t k = ti; k <= text_segs.size(); ++k) {
                if (match_segments(text_segs, k, pat_segs, pi)) {
                    return true;
                }
            }
            return false;
        }

        if (ti >= text_segs.size()) return false;

        // Normal segment match using regex
        try {
            std::regex re("^" + glob_segment_to_regex(pat_segs[pi]) + "$",
                          std::regex::icase);
            if (!std::regex_match(text_segs[ti], re)) return false;
        } catch (const std::regex_error&) {
            return false;
        }

        ++pi;
        ++ti;
    }
    return ti == text_segs.size();
}

bool GlobTool::match_glob(const std::string& text, const std::string& pattern) {
    auto text_segs = split_path(text);
    auto pat_segs = split_path(pattern);
    return match_segments(text_segs, 0, pat_segs, 0);
}

std::vector<std::string> GlobTool::split_path(const std::string& path) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : path) {
        if (c == '/') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) parts.push_back(current);
    return parts;
}

ToolOutput GlobTool::execute(const nlohmann::json& input, const ToolContext& ctx) {
    auto pattern = input["pattern"].get<std::string>();
    std::string search_dir = input.value("path", std::string(""));
    if (search_dir.empty()) {
        search_dir = ctx.cwd;
    }

    namespace fs = std::filesystem;

    std::error_code ec;
    if (!fs::is_directory(search_dir, ec) || ec) {
        return {"Error: path is not a directory: " + search_dir, true};
    }

    // Collect matching files
    std::vector<fs::path> matches;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(search_dir)) {
            if (entry.is_directory(ec) || ec) continue;

            auto filename = entry.path().filename().string();
            if (filename.empty()) continue;

            // Skip hidden files (starting with .)
            if (filename[0] == '.') continue;

            // Get relative path from search_dir for pattern matching
            auto rel = fs::relative(entry.path(), search_dir).string();

            if (match_glob(rel, pattern)) {
                matches.push_back(entry.path());
            }

            if (matches.size() >= 500) break;  // Safety limit
        }
    } catch (const fs::filesystem_error& e) {
        return {std::string("Error: ") + e.what(), true};
    }

    // Sort alphabetically
    std::sort(matches.begin(), matches.end());

    std::ostringstream output;
    for (const auto& p : matches) {
        output << p.string() << "\n";
    }

    if (matches.empty()) {
        return {"No files matched pattern: " + pattern, false};
    }

    return {output.str(), false};
}

}  // namespace ccmake
