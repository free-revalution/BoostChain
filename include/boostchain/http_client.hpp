// include/boostchain/http_client.hpp
#pragma once

#include <boostchain/error.hpp>
#include <string>
#include <map>
#include <memory>

namespace boostchain {

struct HttpResponse {
    int status_code;
    std::string body;
    std::map<std::string, std::string> headers;
};

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    HttpClient(HttpClient&&) noexcept;
    HttpClient& operator=(HttpClient&&) noexcept;

    // Set timeout in seconds
    void set_timeout(int seconds);

    // Set custom headers
    void set_header(const std::string& key, const std::string& value);

    // GET request
    HttpResponse get(const std::string& url);

    // POST request
    HttpResponse post(const std::string& url, const std::string& body);

    // POST with custom content type
    HttpResponse post(const std::string& url,
                     const std::string& body,
                     const std::string& content_type);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace boostchain
