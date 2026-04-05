#include "tools/registry.hpp"

namespace ccmake {

bool ToolRegistry::register_tool(std::unique_ptr<ToolBase> tool) {
    if (!tool) return false;

    const auto& def = tool->definition();

    // Check for name conflict
    if (tools_.count(def.name)) return false;

    // Register primary name
    std::string canonical = def.name;
    tools_[canonical] = std::move(tool);

    // Register aliases
    for (const auto& alias : def.aliases) {
        if (!aliases_.count(alias)) {
            aliases_[alias] = canonical;
        }
    }

    return true;
}

ToolBase* ToolRegistry::find(const std::string& name) const {
    // Direct name lookup
    auto it = tools_.find(name);
    if (it != tools_.end()) return it->second.get();

    // Alias lookup
    auto alias_it = aliases_.find(name);
    if (alias_it != aliases_.end()) {
        auto tool_it = tools_.find(alias_it->second);
        if (tool_it != tools_.end()) return tool_it->second.get();
    }

    return nullptr;
}

bool ToolRegistry::has(const std::string& name) const {
    return find(name) != nullptr;
}

std::vector<ToolDefinition> ToolRegistry::all_definitions() const {
    std::vector<ToolDefinition> defs;
    defs.reserve(tools_.size());
    for (const auto& [name, tool] : tools_) {
        defs.push_back(tool->definition());
    }
    return defs;
}

std::vector<ToolDefinition> ToolRegistry::enabled_definitions() const {
    std::vector<ToolDefinition> defs;
    for (const auto& [name, tool] : tools_) {
        const auto& def = tool->definition();
        if (def.enabled) {
            defs.push_back(def);
        }
    }
    return defs;
}

std::vector<ToolBase*> ToolRegistry::all_tools() const {
    std::vector<ToolBase*> result;
    result.reserve(tools_.size());
    for (const auto& [name, tool] : tools_) {
        result.push_back(tool.get());
    }
    return result;
}

std::vector<ToolDefinition> ToolRegistry::filter_definitions(const std::vector<std::string>& names) const {
    std::vector<ToolDefinition> defs;
    for (const auto& name : names) {
        auto* tool = find(name);
        if (tool) {
            defs.push_back(tool->definition());
        }
    }
    return defs;
}

void ToolRegistry::remove(const std::string& name) {
    tools_.erase(name);
    // Remove aliases pointing to this name
    for (auto it = aliases_.begin(); it != aliases_.end(); ) {
        if (it->second == name) {
            it = aliases_.erase(it);
        } else {
            ++it;
        }
    }
}

void ToolRegistry::clear() {
    tools_.clear();
    aliases_.clear();
}

size_t ToolRegistry::size() const {
    return tools_.size();
}

}  // namespace ccmake
