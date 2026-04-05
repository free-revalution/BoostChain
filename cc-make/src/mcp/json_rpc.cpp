#include "mcp/json_rpc.hpp"
#include "core/json_utils.hpp"

#include <nlohmann/json.hpp>
#include <sstream>
#include <atomic>
#include <regex>

namespace ccmake {

namespace {

std::atomic<int> next_id_{1};

}  // namespace

std::string generate_request_id() {
    return std::to_string(next_id_.fetch_add(1));
}

std::string encode_request(const JsonRpcRequest& req) {
    nlohmann::json j;
    j["jsonrpc"] = req.jsonrpc;
    j["id"] = req.id;
    j["method"] = req.method;
    if (!req.params.is_null()) {
        j["params"] = req.params;
    }
    return j.dump();
}

std::string encode_notification(const JsonRpcNotification& notif) {
    nlohmann::json j;
    j["jsonrpc"] = notif.jsonrpc;
    j["method"] = notif.method;
    if (!notif.params.is_null()) {
        j["params"] = notif.params;
    }
    return j.dump();
}

Result<JsonRpcMessage, std::string> decode_message(const std::string& json_str) {
    try {
        auto j = nlohmann::json::parse(json_str);

        // Must have jsonrpc field
        if (!j.contains("jsonrpc") || j["jsonrpc"].get<std::string>() != "2.0") {
            return Result<JsonRpcMessage, std::string>::err("Invalid or missing jsonrpc version");
        }

        // Has "id" field → response or request
        // No "id" → notification
        bool has_id = j.contains("id");
        bool has_method = j.contains("method");
        bool has_result = j.contains("result");
        bool has_error = j.contains("error");

        if (has_id && has_result) {
            // Response with result
            JsonRpcResponse resp;
            resp.jsonrpc = "2.0";
            resp.id = j["id"];
            resp.result = j["result"];
            return Result<JsonRpcMessage, std::string>::ok(resp);
        }

        if (has_id && has_error) {
            // Response with error
            JsonRpcResponse resp;
            resp.jsonrpc = "2.0";
            resp.id = j["id"];
            JsonRpcError err;
            err.code = j["error"]["code"].get<int>();
            err.message = j["error"]["message"].get<std::string>();
            if (j["error"].contains("data") && !j["error"]["data"].is_null()) {
                err.data = j["error"]["data"];
            }
            resp.error = std::move(err);
            return Result<JsonRpcMessage, std::string>::ok(resp);
        }

        if (has_id && has_method) {
            // Request (server → client request, e.g., tools/list)
            JsonRpcRequest req;
            req.jsonrpc = "2.0";
            if (j["id"].is_string()) {
                req.id = j["id"].get<std::string>();
            } else if (j["id"].is_number_integer()) {
                req.id = std::to_string(j["id"].get<int>());
            } else {
                req.id = j["id"].dump();
            }
            req.method = j["method"].get<std::string>();
            if (j.contains("params") && !j["params"].is_null()) {
                req.params = j["params"];
            }
            return Result<JsonRpcMessage, std::string>::ok(req);
        }

        if (has_method && !has_id) {
            // Notification
            JsonRpcNotification notif;
            notif.jsonrpc = "2.0";
            notif.method = j["method"].get<std::string>();
            if (j.contains("params") && !j["params"].is_null()) {
                notif.params = j["params"];
            }
            return Result<JsonRpcMessage, std::string>::ok(notif);
        }

        return Result<JsonRpcMessage, std::string>::err("Cannot determine message type");
    } catch (const nlohmann::json::parse_error& e) {
        return Result<JsonRpcMessage, std::string>::err(
            std::string("JSON parse error: ") + e.what());
    } catch (const std::exception& e) {
        return Result<JsonRpcMessage, std::string>::err(
            std::string("Decode error: ") + e.what());
    }
}

std::string frame_message(const std::string& json_body) {
    std::ostringstream oss;
    oss << "Content-Length: " << json_body.size() << "\r\n\r\n" << json_body;
    return oss.str();
}

std::optional<std::pair<std::string, size_t>> parse_framed_message(const std::string& buffer) {
    // Look for "Content-Length: N\r\n\r\n"
    // We may also have other headers before the blank line
    auto header_end = buffer.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return std::nullopt;  // Incomplete headers
    }

    // Parse Content-Length from headers
    std::string headers = buffer.substr(0, header_end);
    int content_length = -1;

    // Simple header parsing
    std::istringstream hs(headers);
    std::string line;
    while (std::getline(hs, line)) {
        // Remove trailing \r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.find("Content-Length:") == 0) {
            std::string val = line.substr(15);
            // Trim whitespace
            size_t start = val.find_first_not_of(" \t");
            if (start != std::string::npos) {
                val = val.substr(start);
            }
            try {
                content_length = std::stoi(val);
            } catch (...) {
                return std::nullopt;
            }
            break;
        }
    }

    if (content_length < 0) {
        return std::nullopt;
    }

    size_t body_start = header_end + 4;  // Skip "\r\n\r\n"
    if (buffer.size() < body_start + static_cast<size_t>(content_length)) {
        return std::nullopt;  // Incomplete body
    }

    std::string body = buffer.substr(body_start, content_length);
    size_t total_consumed = body_start + content_length;
    return std::make_pair(std::move(body), total_consumed);
}

}  // namespace ccmake
