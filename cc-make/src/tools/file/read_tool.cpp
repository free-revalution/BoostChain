#include "tools/file/read_tool.hpp"
#include "core/error.hpp"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <iomanip>

namespace ccmake {

ReadTool::ReadTool() = default;

const ToolDefinition& ReadTool::definition() const {
    static ToolDefinition def = [] {
        ToolDefinition d;
        d.name = "Read";
        d.description = "Read a file from the local filesystem.";
        d.input_schema = {
            {"type", "object"},
            {"properties", {
                {"file_path", {{"type", "string"}, {"description", "The absolute path to the file to read"}}},
                {"offset", {{"type", "integer"}, {"description", "The line number to start reading from (1-based)"}}},
                {"limit", {{"type", "integer"}, {"description", "The number of lines to read"}}}
            }},
            {"required", {"file_path"}}
        };
        d.aliases = {"FileRead"};
        d.is_read_only = true;
        d.is_concurrency_safe = true;
        d.enabled = true;
        return d;
    }();
    return def;
}

std::string ReadTool::validate_input(const nlohmann::json& input, const ToolContext& ctx) const {
    if (!input.contains("file_path") || !input["file_path"].is_string()) {
        return "file_path is required and must be a string";
    }
    auto path = input["file_path"].get<std::string>();
    if (path.empty()) {
        return "file_path must not be empty";
    }
    return "";
}

ToolOutput ReadTool::execute(const nlohmann::json& input, const ToolContext& ctx) {
    auto file_path = input["file_path"].get<std::string>();
    int offset = input.value("offset", 1);
    int limit = input.value("limit", 2000);
    int max_lines = 100000;

    if (offset < 1) offset = 1;

    std::ifstream file(file_path);
    if (!file.is_open()) {
        return {std::string("Error: cannot open file: ") + file_path, true};
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
        if (static_cast<int>(lines.size()) > max_lines) break;
    }

    if (lines.empty()) {
        return {"", false};
    }

    int total_lines = static_cast<int>(lines.size());
    if (offset > total_lines) offset = total_lines;
    int end_line = std::min(offset + limit - 1, total_lines);

    std::ostringstream output;
    int width = static_cast<int>(std::to_string(end_line).size());
    for (int i = offset - 1; i < end_line; ++i) {
        output << std::setw(width) << std::right << (i + 1) << "\t" << lines[i] << "\n";
    }

    return {output.str(), false};
}

}  // namespace ccmake
