#pragma once

#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <memory>

namespace ccmake {

struct HTTPTransportConfig {
    std::string base_url = "https://api.anthropic.com";
    std::string api_key;
    std::string api_version = "2023-06-01";
    int timeout_ms = 600000;
    int max_connect_timeout_ms = 30000;
    std::vector<std::string> betas;
    std::string user_agent = "cc-make/0.1.0";
    std::vector<std::pair<std::string, std::string>> extra_headers;

    std::vector<std::pair<std::string, std::string>> build_headers() const;
};

struct HTTPTransportResponse {
    int status_code = 0;
    std::string body;
    std::vector<std::pair<std::string, std::string>> headers;
};

using StreamCallback = std::function<void(const std::string& chunk)>;
using ErrorCallback = std::function<void(const std::string& error)>;

class HTTPTransport {
public:
    explicit HTTPTransport(HTTPTransportConfig config);
    ~HTTPTransport();

    HTTPTransport(const HTTPTransport&) = delete;
    HTTPTransport& operator=(const HTTPTransport&) = delete;

    HTTPTransportResponse post(const std::string& path, const std::string& json_body);
    HTTPTransportResponse post_streaming(
        const std::string& path,
        const std::string& json_body,
        StreamCallback on_chunk,
        ErrorCallback on_error = nullptr
    );
    void abort();
    const HTTPTransportConfig& config() const { return config_; }

private:
    HTTPTransportConfig config_;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace ccmake
