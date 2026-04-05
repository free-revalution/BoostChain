#include "tools/search/grep_tool.hpp"
#include "core/error.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

namespace ccmake {

GrepTool::GrepTool() = default;

const ToolDefinition& GrepTool::definition() const {
    static ToolDefinition def = [] {
        ToolDefinition d;
        d.name = "Grep";
        d.description = "Search file contents using a regular expression pattern.";
        d.input_schema = {
            {"type", "object"},
            {"properties", {
                {"pattern", {{"type", "string"}, {"description", "The regex pattern to search for"}}},
                {"path", {{"type", "string"}, {"description", "The directory to search in (default: cwd)"}}}
            }},
            {"required", {"pattern"}}
        };
        d.aliases = {"Search", "ContentSearch"};
        d.is_read_only = true;
        d.is_concurrency_safe = true;
        return d;
    }();
    return def;
}

std::string GrepTool::validate_input(const nlohmann::json& input, const ToolContext& ctx) const {
    if (!input.contains("pattern") || !input["pattern"].is_string()) {
        return "pattern is required and must be a string";
    }
    if (input["pattern"].get<std::string>().empty()) {
        return "pattern must not be empty";
    }
    return "";
}

struct GrepResult {
    std::string file_path;
    int line_number;
    std::string line_content;
};

static std::vector<GrepResult> grep_file(const std::filesystem::path& filepath,
                                         pcre2_code* re, pcre2_match_data* match_data) {
    std::vector<GrepResult> results;

    std::ifstream file(filepath);
    if (!file.is_open()) return results;

    std::string line;
    int line_num = 0;
    while (std::getline(file, line)) {
        ++line_num;

        int rc = pcre2_match(re,
                             reinterpret_cast<PCRE2_SPTR>(line.data()),
                             static_cast<PCRE2_SIZE>(line.size()),
                             0, 0, match_data, nullptr);

        if (rc > 0) {
            results.push_back({filepath.string(), line_num, line});
        }
    }

    return results;
}

ToolOutput GrepTool::execute(const nlohmann::json& input, const ToolContext& ctx) {
    auto pattern = input["pattern"].get<std::string>();
    std::string search_dir = input.value("path", std::string(""));
    if (search_dir.empty()) {
        search_dir = ctx.cwd;
    }

    namespace fs = std::filesystem;

    // Validate search directory
    std::error_code ec;
    if (!fs::is_directory(search_dir, ec) || ec) {
        return {"Error: path is not a directory: " + search_dir, true};
    }

    // Compile PCRE2 pattern
    uint32_t pcre_opts = PCRE2_UTF;
    if (input.value("case_insensitive", false)) {
        pcre_opts |= PCRE2_CASELESS;
    }

    int errorcode = 0;
    PCRE2_SIZE erroroffset = 0;
    pcre2_code* re = pcre2_compile(
        reinterpret_cast<PCRE2_SPTR>(pattern.data()),
        pattern.size(), pcre_opts,
        &errorcode, &erroroffset, nullptr);

    if (!re) {
        std::ostringstream err;
        err << "Error: invalid regex at offset " << erroroffset;
        // Get error message
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(errorcode, buffer, sizeof(buffer));
        err << ": " << buffer;
        return {err.str(), true};
    }

    pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(re, nullptr);

    // Collect all matches
    std::vector<GrepResult> all_results;
    int file_count = 0;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(search_dir)) {
            if (entry.is_directory(ec) || ec) continue;

            auto filepath = entry.path();
            auto filename = filepath.filename().string();
            if (filename.empty() || filename[0] == '.') continue;

            // Skip binary files (check extension)
            auto ext = filepath.extension().string();
            std::vector<std::string> binary_exts = {
                ".png", ".jpg", ".jpeg", ".gif", ".bmp", ".ico", ".webp",
                ".zip", ".tar", ".gz", ".bz2", ".xz", ".7z", ".rar",
                ".pdf", ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx",
                ".exe", ".dll", ".so", ".dylib", ".o", ".a", ".lib",
                ".mp3", ".mp4", ".avi", ".mov", ".wav", ".flac",
                ".wasm", ".class", ".pyc"
            };
            bool is_binary = false;
            for (const auto& be : binary_exts) {
                if (ext == be) { is_binary = true; break; }
            }
            if (is_binary) continue;

            auto results = grep_file(filepath, re, match_data);
            if (!results.empty()) {
                ++file_count;
            }
            for (auto& r : results) {
                all_results.push_back(std::move(r));
            }

            if (all_results.size() >= 500) break;  // Safety limit
        }
    } catch (const fs::filesystem_error& e) {
        pcre2_match_data_free(match_data);
        pcre2_code_free(re);
        return {std::string("Error: ") + e.what(), true};
    }

    pcre2_match_data_free(match_data);
    pcre2_code_free(re);

    if (all_results.empty()) {
        return {"No matches found for pattern: " + pattern, false};
    }

    // Format output: filepath:line:content
    std::ostringstream output;
    for (const auto& r : all_results) {
        output << r.file_path << ":" << r.line_number << ":" << r.line_content << "\n";
    }

    // Add summary
    output << "\n" << all_results.size() << " matches in " << file_count << " files\n";

    return {output.str(), false};
}

}  // namespace ccmake
