#pragma once

#include "mcp/types.hpp"
#include "core/result.hpp"

#include <string>
#include <memory>
#include <chrono>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include <thread>
#include <unordered_map>
#include <optional>

namespace ccmake {

// ============================================================
// Abstract MCP Transport
// ============================================================

class McpTransport {
public:
    virtual ~McpTransport() = default;

    // Send a request and wait for the response
    virtual Result<nlohmann::json, std::string> send_request(
        const JsonRpcRequest& req,
        std::chrono::milliseconds timeout = std::chrono::seconds(30)
    ) = 0;

    // Send a notification (no response expected). Returns empty string on success.
    virtual Result<std::string, std::string> send_notification(
        const JsonRpcNotification& notif
    ) = 0;

    // Close the transport and clean up resources
    virtual void close() = 0;

    // Check if the transport is connected
    virtual bool is_connected() const = 0;
};

// ============================================================
// Stdio Transport — launches child process, communicates via stdin/stdout
// ============================================================

class StdioTransport : public McpTransport {
public:
    StdioTransport(
        const std::string& command,
        const std::vector<std::string>& args = {},
        const std::vector<std::pair<std::string, std::string>>& env = {}
    );
    ~StdioTransport() override;

    StdioTransport(const StdioTransport&) = delete;
    StdioTransport& operator=(const StdioTransport&) = delete;

    Result<nlohmann::json, std::string> send_request(
        const JsonRpcRequest& req,
        std::chrono::milliseconds timeout = std::chrono::seconds(30)
    ) override;

    Result<std::string, std::string> send_notification(
        const JsonRpcNotification& notif
    ) override;

    void close() override;
    bool is_connected() const override;

private:
    void start_reader_thread();
    void reader_loop();
    void stop_reader_thread();

    std::string command_;
    std::vector<std::string> args_;
    std::vector<std::pair<std::string, std::string>> env_;

    std::unique_ptr<class AsyncProcess> process_;
    std::thread reader_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};

    // Pending requests: id -> promise
    mutable std::mutex pending_mutex_;
    std::unordered_map<std::string, std::promise<nlohmann::json>> pending_requests_;
    std::unordered_map<std::string, std::promise<nlohmann::json>> pending_errors_;

    // Read buffer for Content-Length framing
    std::string read_buffer_;
};

// ============================================================
// SSE Transport — communicates via HTTP POST + SSE stream
// ============================================================

class SseTransport : public McpTransport {
public:
    explicit SseTransport(const std::string& url);
    ~SseTransport() override = default;

    SseTransport(const SseTransport&) = delete;
    SseTransport& operator=(const SseTransport&) = delete;

    Result<nlohmann::json, std::string> send_request(
        const JsonRpcRequest& req,
        std::chrono::milliseconds timeout = std::chrono::seconds(30)
    ) override;

    Result<std::string, std::string> send_notification(
        const JsonRpcNotification& notif
    ) override;

    void close() override;
    bool is_connected() const override;

private:
    std::string url_;
    bool connected_ = false;
};

}  // namespace ccmake
