#pragma once

#include "mcp/transport.hpp"
#include "mcp/types.hpp"
#include "core/result.hpp"

#include <memory>
#include <string>
#include <vector>

namespace ccmake {

class McpClient {
public:
    explicit McpClient(
        std::string server_name,
        std::unique_ptr<McpTransport> transport
    );
    ~McpClient();

    McpClient(const McpClient&) = delete;
    McpClient& operator=(const McpClient&) = delete;

    // Connect to the MCP server (send initialize + initialized notification)
    // Returns true on success
    Result<bool, std::string> connect();

    // Disconnect and clean up
    void disconnect();

    // Check if connected
    bool is_connected() const;

    // List available tools from the server
    Result<std::vector<McpToolDefinition>, std::string> list_tools();

    // Call a tool on the server
    Result<nlohmann::json, std::string> call_tool(
        const std::string& tool_name,
        const nlohmann::json& arguments
    );

    // List available resources
    Result<std::vector<McpResource>, std::string> list_resources();

    // Read a resource
    Result<std::vector<McpResourceContents>, std::string> read_resource(
        const std::string& uri
    );

    // Server info
    const std::string& server_name() const { return server_name_; }
    McpServerCapabilities capabilities() const { return capabilities_; }

private:
    std::string server_name_;
    std::unique_ptr<McpTransport> transport_;
    McpServerCapabilities capabilities_;
    bool connected_ = false;

    // Send a JSON-RPC request through the transport
    Result<nlohmann::json, std::string> send_request(const JsonRpcRequest& req);

    // Send a JSON-RPC notification through the transport
    Result<std::string, std::string> send_notification(const JsonRpcNotification& notif);
};

}  // namespace ccmake
