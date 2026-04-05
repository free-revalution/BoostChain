#pragma once

#include "mcp/types.hpp"
#include "core/result.hpp"

#include <string>

namespace ccmake {

// Encode a JSON-RPC request to a JSON string
std::string encode_request(const JsonRpcRequest& req);

// Encode a JSON-RPC notification to a JSON string
std::string encode_notification(const JsonRpcNotification& notif);

// Decode a JSON-RPC message string into a variant
Result<JsonRpcMessage, std::string> decode_message(const std::string& json_str);

// Create Content-Length framed message for stdio transport
// Returns "Content-Length: N\r\n\r\n{json_body}"
std::string frame_message(const std::string& json_body);

// Parse a Content-Length framed message from a buffer
// Returns {message_body, bytes_consumed} or nullopt if incomplete
std::optional<std::pair<std::string, size_t>> parse_framed_message(const std::string& buffer);

// Generate a unique request ID
std::string generate_request_id();

}  // namespace ccmake
