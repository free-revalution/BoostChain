#include "tools/file/write_tool.hpp"

#include <fstream>
#include <sstream>
#include <filesystem>

namespace ccmake {

WriteTool::WriteTool() = default;

const ToolDefinition& WriteTool::definition() const {
    static ToolDefinition def = [] {
        ToolDefinition d;
        d.name = "Write";
        d.description = "Write content to a file, creating it if it doesn't exist or overwriting it.";
        d.input_schema = {
            {"type", "object"},
            {"properties", {
                {"file_path", {{"type", "string"}, {"description", "The absolute path to the file to write"}}},
                {"content", {{"type", "string"}, {"description", "The content to write to the file"}}}
            }},
            {"required", {"file_path", "content"}}
        };
        d.aliases = {"FileWrite"};
        d.is_destructive = false;
        d.enabled = true;
        return d;
    }();
    return def;
}

std::string WriteTool::validate_input(const nlohmann::json& input, const ToolContext& ctx) const {
    if (!input.contains("file_path") || !input["file_path"].is_string()) {
        return "file_path is required and must be a string";
    }
    if (!input.contains("content") || !input["content"].is_string()) {
        return "content is required and must be a string";
    }
    return "";
}

ToolOutput WriteTool::execute(const nlohmann::json& input, const ToolContext& ctx) {
    auto file_path = input["file_path"].get<std::string>();
    auto content = input["content"].get<std::string>();

    // Ensure parent directory exists
    std::filesystem::path p(file_path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    bool is_new = !std::filesystem::exists(file_path);

    std::ofstream out(file_path);
    if (!out.is_open()) {
        return {std::string("Error: cannot write to file: ") + file_path, true};
    }
    out << content;

    std::ostringstream result_ss;
    result_ss << (is_new ? "Created" : "Updated") << " " << file_path << "\n";

    return {result_ss.str(), false};
}

}  // namespace ccmake
