#include "api/http_transport.hpp"

#include <curl/curl.h>
#include <sstream>
#include <algorithm>

namespace ccmake {

// ============================================================
// HTTPTransportConfig
// ============================================================

std::vector<std::pair<std::string, std::string>> HTTPTransportConfig::build_headers() const {
    std::vector<std::pair<std::string, std::string>> headers;

    headers.emplace_back("Content-Type", "application/json");
    headers.emplace_back("Accept", "text/event-stream");
    headers.emplace_back("anthropic-version", api_version);
    headers.emplace_back("User-Agent", user_agent);

    if (!api_key.empty()) {
        headers.emplace_back("x-api-key", api_key);
    }

    if (!betas.empty()) {
        std::string beta_value;
        for (size_t i = 0; i < betas.size(); ++i) {
            if (i > 0) beta_value += ",";
            beta_value += betas[i];
        }
        headers.emplace_back("anthropic-beta", beta_value);
    }

    for (const auto& [k, v] : extra_headers) {
        headers.emplace_back(k, v);
    }

    return headers;
}

// ============================================================
// HTTPTransport::Impl — holds mutable curl state
// ============================================================

struct HTTPTransport::Impl {
    bool abort_flag = false;
};

// ============================================================
// libcurl write/header callback adapters
// ============================================================

namespace {

struct WriteContext {
    std::string* body;
    bool* abort_flag;
    StreamCallback stream_cb;
    bool is_streaming;
};

size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* ctx = static_cast<WriteContext*>(userdata);
    size_t total = size * nmemb;

    if (ctx->abort_flag && *ctx->abort_flag) {
        return 0;  // abort transfer
    }

    std::string chunk(ptr, total);

    if (ctx->is_streaming && ctx->stream_cb) {
        ctx->stream_cb(chunk);
    }

    if (ctx->body) {
        ctx->body->append(chunk);
    }

    return total;
}

struct HeaderContext {
    std::vector<std::pair<std::string, std::string>>* headers;
};

size_t header_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* ctx = static_cast<HeaderContext*>(userdata);
    size_t total = size * nmemb;

    std::string line(ptr, total);
    // Strip trailing \r\n
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
        line.pop_back();
    }

    // Skip empty lines and HTTP status line (e.g. "HTTP/2 200")
    if (line.empty() || line.find("HTTP/") == 0) {
        return total;
    }

    auto colon = line.find(':');
    if (colon != std::string::npos) {
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        // Trim leading whitespace from value
        size_t start = value.find_first_not_of(" \t");
        if (start != std::string::npos) {
            value = value.substr(start);
        }

        ctx->headers->emplace_back(key, value);
    }

    return total;
}

}  // anonymous namespace

// ============================================================
// HTTPTransport
// ============================================================

HTTPTransport::HTTPTransport(HTTPTransportConfig config)
    : config_(std::move(config))
    , impl_(std::make_unique<Impl>()) {}

HTTPTransport::~HTTPTransport() = default;

HTTPTransportResponse HTTPTransport::post(const std::string& path, const std::string& json_body) {
    HTTPTransportResponse response;
    CURL* curl = curl_easy_init();
    if (!curl) {
        return response;
    }

    std::string url = config_.base_url + path;

    // Build headers as curl_slist
    curl_slist* slist = nullptr;
    auto headers = config_.build_headers();
    for (const auto& [k, v] : headers) {
        std::string header_line = k + ": " + v;
        slist = curl_slist_append(slist, header_line.c_str());
    }

    // Write context (non-streaming)
    WriteContext write_ctx;
    write_ctx.body = &response.body;
    write_ctx.abort_flag = &impl_->abort_flag;
    write_ctx.is_streaming = false;

    // Header context
    HeaderContext header_ctx;
    header_ctx.headers = &response.headers;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_ctx);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_ctx);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(config_.timeout_ms));
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(config_.max_connect_timeout_ms));

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        response.body = curl_easy_strerror(res);
    } else {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        response.status_code = static_cast<int>(http_code);
    }

    curl_slist_free_all(slist);
    curl_easy_cleanup(curl);

    return response;
}

HTTPTransportResponse HTTPTransport::post_streaming(
    const std::string& path,
    const std::string& json_body,
    StreamCallback on_chunk,
    ErrorCallback on_error
) {
    HTTPTransportResponse response;
    CURL* curl = curl_easy_init();
    if (!curl) {
        if (on_error) {
            on_error("Failed to initialize curl handle");
        }
        return response;
    }

    std::string url = config_.base_url + path;

    // Build headers as curl_slist
    curl_slist* slist = nullptr;
    auto headers = config_.build_headers();
    for (const auto& [k, v] : headers) {
        std::string header_line = k + ": " + v;
        slist = curl_slist_append(slist, header_line.c_str());
    }

    // Write context (streaming)
    WriteContext write_ctx;
    write_ctx.body = &response.body;
    write_ctx.abort_flag = &impl_->abort_flag;
    write_ctx.stream_cb = std::move(on_chunk);
    write_ctx.is_streaming = true;

    // Header context
    HeaderContext header_ctx;
    header_ctx.headers = &response.headers;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_ctx);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_ctx);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(config_.timeout_ms));
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(config_.max_connect_timeout_ms));

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK && res != CURLE_ABORTED_BY_CALLBACK) {
        std::string err_msg = curl_easy_strerror(res);
        response.body = err_msg;
        if (on_error) {
            on_error(err_msg);
        }
    } else {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        response.status_code = static_cast<int>(http_code);
    }

    curl_slist_free_all(slist);
    curl_easy_cleanup(curl);

    return response;
}

void HTTPTransport::abort() {
    impl_->abort_flag = true;
}

}  // namespace ccmake
