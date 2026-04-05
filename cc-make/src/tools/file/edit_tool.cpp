#include "tools/file/edit_tool.hpp"
#include "core/error.hpp"

#include <fstream>
#include <sstream>
#include <filesystem>

namespace ccmake {

EditTool::EditTool() = default;

const ToolDefinition& EditTool::definition() const {
    static ToolDefinition def = [] {
        ToolDefinition d;
        d.name = "Edit";
        d.description = "Edit a file by replacing an exact string match with new content.";
        d.input_schema = {
            {"type", "object"},
            {"properties", {
                {"file_path", {{"type", "string"}, {"description", "The absolute path to the file to modify"}}},
                {"old_string", {{"type", "string"}, {"description", "The text to replace"}}},
                {"new_string", {{"type", "string"}, {"description", "The text to replace it with"}}},
                {"replace_all", {{"type", "boolean"}, {"default", false}}}
            }},
            {"required", {"file_path", "old_string", "new_string"}}
        };
        d.aliases = {"FileEdit"};
        d.is_destructive = false;
        d.enabled = true;
        return d;
    }();
    return def;
}

std::string EditTool::validate_input(const nlohmann::json& input, const ToolContext& ctx) const {
    if (!input.contains("file_path") || !input["file_path"].is_string() ||
        !input.contains("old_string") || !input["old_string"].is_string() ||
        !input.contains("new_string") || !input["new_string"].is_string()) {
        return "file_path, old_string, and new_string are required";
    }

    std::string old_str = input["old_string"].get<std::string>();
    std::string new_str = input["new_string"].get<std::string>();
    if (old_str.empty()) {
        return "old_string must not be empty";
    }
    if (old_str == new_str) {
        return "old_string and new_string must be different";
    }

    return "";
}

ToolOutput EditTool::execute(const nlohmann::json& input, const ToolContext& ctx) {
    auto file_path = input["file_path"].get<std::string>();
    auto old_string = input["old_string"].get<std::string>();
    auto new_string = input["new_string"].get<std::string>();
    bool replace_all = input.value("replace_all", false);

    std::ifstream file(file_path);
    if (!file.is_open()) {
        return {std::string("Error: cannot open file: ") + file_path, true};
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();
    file.close();

    // Find the old_string
    size_t pos = content.find(old_string);
    if (pos == std::string::npos) {
        return {"Error: old_string not found in file", true};
    }

    if (!replace_all) {
        // Check for multiple matches
        size_t second = content.find(old_string, pos + old_string.size());
        if (second != std::string::npos) {
            return {"Error: old_string matches multiple locations. Use replace_all=true or provide more context.", true};
        }
    }

    // Perform replacement
    if (replace_all) {
        size_t start = 0;
        while ((pos = content.find(old_string, start)) != std::string::npos) {
            content.replace(pos, old_string.size(), new_string);
            start = pos + new_string.size();
        }
    } else {
        content.replace(pos, old_string.size(), new_string);
    }

    // Write back
    std::ofstream out(file_path);
    if (!out.is_open()) {
        return {std::string("Error: cannot write to file: ") + file_path, true};
    }
    out << content;

    std::ostringstream result_ss;
    result_ss << "Successfully edited " << file_path << "\n";
    result_ss << "Replaced " << (replace_all ? "all occurrences" : "1 occurrence") << " of old_string";

    return {result_ss.str(), false};
}

}  // namespace ccmake
