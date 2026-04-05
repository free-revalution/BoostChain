#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <unordered_map>

namespace ccmake {

// ============================================================
// JSON-RPC 2.0 Message Types
// ============================================================

struct JsonRpcError {
    int code;
    std::string message;
    std::optional<nlohmann::json> data;
};

struct JsonRpcRequest {
    std::string jsonrpc = "2.0";
    std::string id;
    std::string method;
    nlohmann::json params;
};

struct JsonRpcResponse {
    std::string jsonrpc = "2.0";
    nlohmann::json id;    // string, int, or null
    std::optional<nlohmann::json> result;
    std::optional<JsonRpcError> error;
};

struct JsonRpcNotification {
    std::string jsonrpc = "2.0";
    std::string method;
    nlohmann::json params;
};

using JsonRpcMessage = std::variant<JsonRpcRequest, JsonRpcResponse, JsonRpcNotification>;

// ============================================================
// MCP Protocol Types
// ============================================================

struct McpToolDefinition {
    std::string name;
    std::string description;
    nlohmann::json input_schema;
};

struct McpResource {
    std::string uri;
    std::string name;
    std::string description;
    std::string mime_type;
};

struct McpResourceContents {
    std::string uri;
    std::string mime_type;
    std::string text;
};

struct McpServerCapabilities {
    bool tools = false;
    bool resources = false;
    bool prompts = false;
};

struct McpServerConfig {
    std::string name;
    std::string command;            // stdio: command to run
    std::vector<std::string> args;  // stdio: command arguments
    std::vector<std::pair<std::string, std::string>> env;  // stdio: env vars
    std::string url;                // sse/http: URL to connect to
    bool disabled = false;
};

// ============================================================
// MCP Config (YAML file structure)
// ============================================================

struct McpConfig {
    std::unordered_map<std::string, McpServerConfig> servers;
};

// Standard MCP JSON-RPC error codes
namespace McpErrorCode {
    constexpr int ParseError = -32700;
    constexpr int InvalidRequest = -32600;
    constexpr int MethodNotFound = -32601;
    constexpr int InvalidParams = -32602;
    constexpr int InternalError = -32603;
}

}  // namespace ccmake
