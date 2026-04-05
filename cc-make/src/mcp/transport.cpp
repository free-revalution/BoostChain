#include "mcp/transport.hpp"
#include "mcp/json_rpc.hpp"
#include "utils/process.hpp"

#include <spdlog/spdlog.h>

#include <sstream>
#include <future>

namespace ccmake {

// ============================================================
// Stdio Transport Implementation
// ============================================================

StdioTransport::StdioTransport(
    const std::string& command,
    const std::vector<std::string>& args,
    const std::vector<std::pair<std::string, std::string>>& env
) : command_(command), args_(args), env_(env) {}

StdioTransport::~StdioTransport() {
    close();
}

void StdioTransport::start_reader_thread() {
    std::string full_cmd = command_;
    for (const auto& arg : args_) {
        full_cmd += " " + arg;
    }

    process_ = run_command_async(full_cmd, std::nullopt, env_);
    if (!process_) {
        spdlog::error("Failed to start MCP server process: {}", command_);
        return;
    }

    running_ = true;
    connected_ = true;
    reader_thread_ = std::thread(&StdioTransport::reader_loop, this);
}

void StdioTransport::reader_loop() {
    while (running_) {
        auto data = process_->read_stdout();
        if (!data.has_value()) {
            break;
        }
        if (data->empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        read_buffer_ += *data;

        while (true) {
            auto parsed = parse_framed_message(read_buffer_);
            if (!parsed.has_value()) break;

            auto& [body, consumed] = *parsed;
            read_buffer_ = read_buffer_.substr(consumed);

            auto msg_result = decode_message(body);
            if (!msg_result.has_value()) {
                spdlog::warn("Failed to decode MCP message: {}", msg_result.error());
                continue;
            }

            auto& msg = msg_result.value();

            if (auto* resp = std::get_if<JsonRpcResponse>(&msg)) {
                std::string id;
                if (resp->id.is_string()) {
                    id = resp->id.get<std::string>();
                } else {
                    id = resp->id.dump();
                }

                std::lock_guard<std::mutex> lock(pending_mutex_);
                auto it = pending_requests_.find(id);
                if (it != pending_requests_.end()) {
                    if (resp->error.has_value()) {
                        it->second.set_value(nlohmann::json({
                            {"error", true},
                            {"code", resp->error->code},
                            {"message", resp->error->message}
                        }));
                    } else if (resp->result.has_value()) {
                        it->second.set_value(*resp->result);
                    } else {
                        it->second.set_value(nlohmann::json::object());
                    }
                    pending_requests_.erase(it);
                }
            }
            else if (auto* notif = std::get_if<JsonRpcNotification>(&msg)) {
                spdlog::debug("MCP notification: {}", notif->method);
            }
        }
    }

    connected_ = false;
}

void StdioTransport::stop_reader_thread() {
    running_ = false;
    if (reader_thread_.joinable()) {
        reader_thread_.join();
    }
}

Result<nlohmann::json, std::string> StdioTransport::send_request(
    const JsonRpcRequest& req,
    std::chrono::milliseconds timeout
) {
    if (!connected_ && !process_) {
        start_reader_thread();
        if (!connected_) {
            return Result<nlohmann::json, std::string>::err("Failed to connect to MCP server");
        }
    }

    std::string id = req.id.empty() ? generate_request_id() : req.id;
    JsonRpcRequest actual_req = req;
    actual_req.id = id;

    std::promise<nlohmann::json> promise;
    auto future = promise.get_future();

    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_requests_[id] = std::move(promise);
    }

    auto encoded = encode_request(actual_req);
    auto framed = frame_message(encoded);

    if (!process_->write_stdin(framed)) {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_requests_.erase(id);
        return Result<nlohmann::json, std::string>::err("Failed to write to MCP server stdin");
    }

    auto status = future.wait_for(timeout);
    if (status == std::future_status::timeout) {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_requests_.erase(id);
        return Result<nlohmann::json, std::string>::err("MCP request timed out");
    }

    auto result = future.get();

    if (result.contains("error") && result["error"].get<bool>()) {
        return Result<nlohmann::json, std::string>::err(
            "MCP error " + std::to_string(result["code"].get<int>()) + ": " +
            result["message"].get<std::string>()
        );
    }

    return Result<nlohmann::json, std::string>::ok(result);
}

Result<std::string, std::string> StdioTransport::send_notification(
    const JsonRpcNotification& notif
) {
    if (!connected_ && !process_) {
        start_reader_thread();
        if (!connected_) {
            return Result<std::string, std::string>::err("Failed to connect to MCP server");
        }
    }

    auto encoded = encode_notification(notif);
    auto framed = frame_message(encoded);

    if (!process_->write_stdin(framed)) {
        return Result<std::string, std::string>::err("Failed to write notification to MCP server");
    }

    return Result<std::string, std::string>::ok("");
}

void StdioTransport::close() {
    stop_reader_thread();
    if (process_) {
        process_->kill();
        process_.reset();
    }
    connected_ = false;
}

bool StdioTransport::is_connected() const {
    return connected_ && process_ && process_->is_running();
}

// ============================================================
// SSE Transport Implementation
// ============================================================

SseTransport::SseTransport(const std::string& url) : url_(url) {}

Result<nlohmann::json, std::string> SseTransport::send_request(
    const JsonRpcRequest& req,
    std::chrono::milliseconds timeout
) {
    (void)timeout;

    std::string json_body = encode_request(req);

    std::ostringstream cmd;
    cmd << "curl -s -X POST '" << url_ << "' "
        << "-H 'Content-Type: application/json' "
        << "-d '" << json_body << "' "
        << "--max-time " << std::chrono::duration_cast<std::chrono::seconds>(timeout).count();

    auto result = run_command(cmd.str(), std::nullopt, timeout);

    if (result.timed_out) {
        return Result<nlohmann::json, std::string>::err("SSE request timed out");
    }
    if (result.exit_code != 0) {
        return Result<nlohmann::json, std::string>::err(
            "SSE request failed: " + result.stderr_output);
    }

    try {
        auto j = nlohmann::json::parse(result.stdout_output);
        return Result<nlohmann::json, std::string>::ok(j);
    } catch (const nlohmann::json::parse_error& e) {
        return Result<nlohmann::json, std::string>::err(
            std::string("Failed to parse SSE response: ") + e.what());
    }
}

Result<std::string, std::string> SseTransport::send_notification(
    const JsonRpcNotification& notif
) {
    std::string json_body = encode_notification(notif);

    std::ostringstream cmd;
    cmd << "curl -s -X POST '" << url_ << "' "
        << "-H 'Content-Type: application/json' "
        << "-d '" << json_body << "' "
        << "--max-time 5";

    auto result = run_command(cmd.str());

    if (result.exit_code != 0) {
        return Result<std::string, std::string>::err(
            "SSE notification failed: " + result.stderr_output);
    }

    connected_ = true;
    return Result<std::string, std::string>::ok("");
}

void SseTransport::close() {
    connected_ = false;
}

bool SseTransport::is_connected() const {
    return connected_;
}

}  // namespace ccmake
