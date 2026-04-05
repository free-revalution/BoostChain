#include "mcp/config.hpp"
#include "mcp/tool_bridge.hpp"
#include "config/config_loader.hpp"

#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <fstream>

namespace ccmake {

// ============================================================
// McpConfigLoader
// ============================================================

Result<McpConfig, std::string> McpConfigLoader::load_from_file(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return Result<McpConfig, std::string>::ok(McpConfig{});
    }

    try {
        YAML::Node root = YAML::LoadFile(path.string());
        McpConfig config;

        if (root["mcpServers"] && root["mcpServers"].IsMap()) {
            for (const auto& entry : root["mcpServers"]) {
                std::string name = entry.first.as<std::string>();
                const auto& node = entry.second;

                McpServerConfig server;
                server.name = name;

                if (node["command"]) {
                    server.command = node["command"].as<std::string>();
                }
                if (node["args"] && node["args"].IsSequence()) {
                    for (const auto& arg : node["args"]) {
                        server.args.push_back(arg.as<std::string>());
                    }
                }
                if (node["env"] && node["env"].IsMap()) {
                    for (const auto& env_entry : node["env"]) {
                        server.env.push_back({
                            env_entry.first.as<std::string>(),
                            env_entry.second.as<std::string>()
                        });
                    }
                }
                if (node["url"]) {
                    server.url = node["url"].as<std::string>();
                }
                if (node["disabled"]) {
                    server.disabled = node["disabled"].as<bool>();
                }

                config.servers[name] = std::move(server);
            }
        }

        return Result<McpConfig, std::string>::ok(std::move(config));
    } catch (const YAML::Exception& e) {
        return Result<McpConfig, std::string>::err(
            std::string("Failed to parse MCP config: ") + e.what());
    }
}

Result<McpConfig, std::string> McpConfigLoader::load_project(const std::filesystem::path& project_dir) {
    auto mcp_yaml = project_dir / ".claude" / "mcp.yaml";
    auto mcp_json = project_dir / ".claude" / "mcp.json";

    // Try YAML first, then JSON
    if (std::filesystem::exists(mcp_yaml)) {
        return load_from_file(mcp_yaml);
    }
    if (std::filesystem::exists(mcp_json)) {
        // JSON format: {"mcpServers": {...}}
        return load_from_file(mcp_json);  // YAML parser handles JSON too
    }

    return Result<McpConfig, std::string>::ok(McpConfig{});
}

Result<McpConfig, std::string> McpConfigLoader::load_user() {
    auto config_dir = get_global_config_dir();
    auto mcp_yaml = config_dir / "mcp.yaml";

    if (std::filesystem::exists(mcp_yaml)) {
        return load_from_file(mcp_yaml);
    }

    return Result<McpConfig, std::string>::ok(McpConfig{});
}

McpConfig McpConfigLoader::merge(const McpConfig& project, const McpConfig& user) {
    McpConfig merged;

    // User servers first (lower priority)
    for (const auto& [name, server] : user.servers) {
        merged.servers[name] = server;
    }

    // Project servers override user servers with same name
    for (const auto& [name, server] : project.servers) {
        merged.servers[name] = server;
    }

    return merged;
}

// ============================================================
// McpManager
// ============================================================

McpManager::McpManager(const std::filesystem::path& project_dir)
    : project_dir_(project_dir) {}

McpManager::~McpManager() {
    disconnect_all();
}

Result<size_t, std::string> McpManager::connect_all() {
    // Load and merge configs
    auto project_config_result = McpConfigLoader::load_project(project_dir_);
    auto user_config_result = McpConfigLoader::load_user();

    if (!project_config_result.has_value()) {
        return Result<size_t, std::string>::err(
            "Failed to load project MCP config: " + project_config_result.error());
    }
    if (!user_config_result.has_value()) {
        return Result<size_t, std::string>::err(
            "Failed to load user MCP config: " + user_config_result.error());
    }

    config_ = McpConfigLoader::merge(project_config_result.value(), user_config_result.value());

    size_t connected = 0;

    for (const auto& [name, server_config] : config_.servers) {
        if (server_config.disabled) {
            spdlog::debug("MCP server disabled: {}", name);
            continue;
        }

        std::unique_ptr<McpTransport> transport;

        if (!server_config.command.empty()) {
            transport = std::make_unique<StdioTransport>(
                server_config.command,
                server_config.args,
                server_config.env
            );
        } else if (!server_config.url.empty()) {
            transport = std::make_unique<SseTransport>(server_config.url);
        } else {
            spdlog::warn("MCP server '{}' has neither command nor url", name);
            continue;
        }

        auto client = std::make_unique<McpClient>(name, std::move(transport));

        auto connect_result = client->connect();
        if (!connect_result.has_value()) {
            spdlog::warn("Failed to connect to MCP server '{}': {}", name, connect_result.error());
            continue;
        }

        auto bridge = std::make_unique<McpToolBridge>(*client);
        auto tools_result = bridge->create_tools();
        if (tools_result.has_value()) {
            spdlog::info("MCP server '{}': {} tools available", name, tools_result.value().size());
        }

        clients_.push_back(std::move(client));
        bridges_.push_back(std::move(bridge));
        ++connected;
    }

    return Result<size_t, std::string>::ok(connected);
}

size_t McpManager::register_tools(ToolRegistry& registry) {
    size_t total = 0;

    for (auto& bridge : bridges_) {
        auto tools_result = bridge->create_tools();
        if (tools_result.has_value()) {
            for (auto& tool : tools_result.value()) {
                registry.register_tool(std::move(tool));
                ++total;
            }
        }
    }

    return total;
}

void McpManager::disconnect_all() {
    for (auto& client : clients_) {
        client->disconnect();
    }
    clients_.clear();
    bridges_.clear();
}

McpClient* McpManager::get_client(const std::string& server_name) {
    for (auto& client : clients_) {
        if (client->server_name() == server_name) {
            return client.get();
        }
    }
    return nullptr;
}

std::vector<std::string> McpManager::connected_servers() const {
    std::vector<std::string> names;
    for (const auto& client : clients_) {
        if (client->is_connected()) {
            names.push_back(client->server_name());
        }
    }
    return names;
}

}  // namespace ccmake
