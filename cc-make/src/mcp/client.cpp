#include "mcp/client.hpp"
#include "mcp/json_rpc.hpp"

#include <spdlog/spdlog.h>

namespace ccmake {

McpClient::McpClient(
    std::string server_name,
    std::unique_ptr<McpTransport> transport
) : server_name_(std::move(server_name)),
    transport_(std::move(transport)) {}

McpClient::~McpClient() {
    disconnect();
}

Result<nlohmann::json, std::string> McpClient::send_request(const JsonRpcRequest& req) {
    return transport_->send_request(req);
}

Result<std::string, std::string> McpClient::send_notification(const JsonRpcNotification& notif) {
    return transport_->send_notification(notif);
}

Result<bool, std::string> McpClient::connect() {
    if (connected_) {
        return Result<bool, std::string>::ok(true);
    }

    // Send initialize request
    JsonRpcRequest init_req;
    init_req.id = generate_request_id();
    init_req.method = "initialize";
    init_req.params = nlohmann::json({
        {"protocolVersion", "2024-11-05"},
        {"capabilities", nlohmann::json::object()},
        {"clientInfo", {
            {"name", "cc-make"},
            {"version", "0.1.0"}
        }}
    });

    auto result = send_request(init_req);
    if (!result.has_value()) {
        return Result<bool, std::string>::err("Initialize failed: " + result.error());
    }

    auto& resp = result.value();

    // Parse server capabilities
    if (resp.contains("capabilities")) {
        auto& caps = resp["capabilities"];
        if (caps.contains("tools")) capabilities_.tools = true;
        if (caps.contains("resources")) capabilities_.resources = true;
        if (caps.contains("prompts")) capabilities_.prompts = true;
    }

    // Send initialized notification
    JsonRpcNotification notif;
    notif.method = "notifications/initialized";
    notif.params = nlohmann::json::object();
    auto notif_result = send_notification(notif);
    if (!notif_result.has_value()) {
        return Result<bool, std::string>::err("Failed to send initialized notification: " + notif_result.error());
    }

    connected_ = true;
    spdlog::info("MCP client connected to server: {}", server_name_);
    return Result<bool, std::string>::ok(true);
}

void McpClient::disconnect() {
    if (connected_) {
        transport_->close();
        connected_ = false;
        spdlog::info("MCP client disconnected from server: {}", server_name_);
    }
}

bool McpClient::is_connected() const {
    return connected_ && transport_->is_connected();
}

Result<std::vector<McpToolDefinition>, std::string> McpClient::list_tools() {
    if (!is_connected()) {
        return Result<std::vector<McpToolDefinition>, std::string>::err("Not connected");
    }

    JsonRpcRequest req;
    req.id = generate_request_id();
    req.method = "tools/list";
    req.params = nlohmann::json::object();

    auto result = send_request(req);
    if (!result.has_value()) {
        return Result<std::vector<McpToolDefinition>, std::string>::err(
            "tools/list failed: " + result.error());
    }

    auto& resp = result.value();
    std::vector<McpToolDefinition> tools;

    if (resp.contains("tools") && resp["tools"].is_array()) {
        for (const auto& t : resp["tools"]) {
            McpToolDefinition def;
            def.name = t.value("name", "");
            def.description = t.value("description", "");
            if (t.contains("inputSchema")) {
                def.input_schema = t["inputSchema"];
            }
            tools.push_back(std::move(def));
        }
    }

    return Result<std::vector<McpToolDefinition>, std::string>::ok(std::move(tools));
}

Result<nlohmann::json, std::string> McpClient::call_tool(
    const std::string& tool_name,
    const nlohmann::json& arguments
) {
    if (!is_connected()) {
        return Result<nlohmann::json, std::string>::err("Not connected");
    }

    JsonRpcRequest req;
    req.id = generate_request_id();
    req.method = "tools/call";
    req.params = nlohmann::json({
        {"name", tool_name},
        {"arguments", arguments}
    });

    auto result = send_request(req);
    if (!result.has_value()) {
        return Result<nlohmann::json, std::string>::err(
            "tools/call failed: " + result.error());
    }

    return result;
}

Result<std::vector<McpResource>, std::string> McpClient::list_resources() {
    if (!is_connected()) {
        return Result<std::vector<McpResource>, std::string>::err("Not connected");
    }

    JsonRpcRequest req;
    req.id = generate_request_id();
    req.method = "resources/list";
    req.params = nlohmann::json::object();

    auto result = send_request(req);
    if (!result.has_value()) {
        return Result<std::vector<McpResource>, std::string>::err(
            "resources/list failed: " + result.error());
    }

    auto& resp = result.value();
    std::vector<McpResource> resources;

    if (resp.contains("resources") && resp["resources"].is_array()) {
        for (const auto& r : resp["resources"]) {
            McpResource res;
            res.uri = r.value("uri", "");
            res.name = r.value("name", "");
            res.description = r.value("description", "");
            res.mime_type = r.value("mimeType", "");
            resources.push_back(std::move(res));
        }
    }

    return Result<std::vector<McpResource>, std::string>::ok(std::move(resources));
}

Result<std::vector<McpResourceContents>, std::string> McpClient::read_resource(
    const std::string& uri
) {
    if (!is_connected()) {
        return Result<std::vector<McpResourceContents>, std::string>::err("Not connected");
    }

    JsonRpcRequest req;
    req.id = generate_request_id();
    req.method = "resources/read";
    req.params = nlohmann::json({{"uri", uri}});

    auto result = send_request(req);
    if (!result.has_value()) {
        return Result<std::vector<McpResourceContents>, std::string>::err(
            "resources/read failed: " + result.error());
    }

    auto& resp = result.value();
    std::vector<McpResourceContents> contents;

    if (resp.contains("contents") && resp["contents"].is_array()) {
        for (const auto& c : resp["contents"]) {
            McpResourceContents cont;
            cont.uri = c.value("uri", "");
            cont.mime_type = c.value("mimeType", "");
            cont.text = c.value("text", "");
            contents.push_back(std::move(cont));
        }
    }

    return Result<std::vector<McpResourceContents>, std::string>::ok(std::move(contents));
}

}  // namespace ccmake
