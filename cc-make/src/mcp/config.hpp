#pragma once

#include "mcp/types.hpp"
#include "mcp/client.hpp"
#include "mcp/tool_bridge.hpp"
#include "tools/registry.hpp"
#include "core/result.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace ccmake {

// Load MCP config from YAML files
class McpConfigLoader {
public:
    // Load MCP server config from a YAML file
    static Result<McpConfig, std::string> load_from_file(const std::filesystem::path& path);

    // Load project-level MCP config (<project>/.claude/mcp.yaml)
    static Result<McpConfig, std::string> load_project(const std::filesystem::path& project_dir);

    // Load user-level MCP config (~/.claude/mcp.yaml)
    static Result<McpConfig, std::string> load_user();

    // Merge two configs (project takes precedence over user for same-named servers)
    static McpConfig merge(const McpConfig& project, const McpConfig& user);
};

// Manages MCP server connections and tool registration
class McpManager {
public:
    explicit McpManager(const std::filesystem::path& project_dir);
    ~McpManager();

    McpManager(const McpManager&) = delete;
    McpManager& operator=(const McpManager&) = delete;

    // Load config and connect to all enabled servers
    Result<size_t, std::string> connect_all();

    // Register all MCP tools into the given tool registry
    size_t register_tools(ToolRegistry& registry);

    // Disconnect all servers
    void disconnect_all();

    // Individual server access
    McpClient* get_client(const std::string& server_name);
    std::vector<std::string> connected_servers() const;

private:
    std::filesystem::path project_dir_;
    McpConfig config_;
    std::vector<std::unique_ptr<McpClient>> clients_;
    std::vector<std::unique_ptr<McpToolBridge>> bridges_;
};

}  // namespace ccmake
