#pragma once

#include "tools/tool.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace ccmake {

class ToolRegistry {
public:
    // Register a tool (takes ownership). Returns false if name conflicts.
    bool register_tool(std::unique_ptr<ToolBase> tool);

    // Find tool by name or alias
    ToolBase* find(const std::string& name) const;

    // Check if a tool exists
    bool has(const std::string& name) const;

    // Get all registered tool definitions (for sending to API)
    std::vector<ToolDefinition> all_definitions() const;

    // Get all enabled tool definitions
    std::vector<ToolDefinition> enabled_definitions() const;

    // Get all registered tools (raw pointers)
    std::vector<ToolBase*> all_tools() const;

    // Filter tools by names (returns definitions for those names)
    std::vector<ToolDefinition> filter_definitions(const std::vector<std::string>& names) const;

    // Remove a tool by name
    void remove(const std::string& name);

    // Clear all tools
    void clear();

    // Number of registered tools
    size_t size() const;

private:
    // name -> tool (primary)
    std::unordered_map<std::string, std::unique_ptr<ToolBase>> tools_;
    // alias -> canonical name
    std::unordered_map<std::string, std::string> aliases_;
};

}  // namespace ccmake
