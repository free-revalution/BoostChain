// src/http_client.cpp
#include <boostchain/http_client.hpp>
#include <httplib.h>
#include <memory>

namespace boostchain {

class HttpClient::Impl {
public:
    struct ParsedUrl {
        std::string host;
        std::string path;
        bool https;
    };

    std::shared_ptr<httplib::Client> client;
    httplib::Headers headers;
    int timeout_sec = 30;

    ParsedUrl parse_url(const std::string& url) {
        ParsedUrl result;
        result.host = url;
        result.path = "/";
        result.https = false;

        size_t proto_pos = result.host.find("://");
        if (proto_pos != std::string::npos) {
            result.https = url.substr(0, 5) == "https";
            result.host = result.host.substr(proto_pos + 3);
        }

        size_t path_pos = result.host.find('/');
        if (path_pos != std::string::npos) {
            result.path = result.host.substr(path_pos);
            result.host = result.host.substr(0, path_pos);
        }

        return result;
    }

    void update_client(const std::string& url) {
        ParsedUrl parsed = parse_url(url);

        client = std::make_shared<httplib::Client>(parsed.host.c_str());
        client->set_connection_timeout(timeout_sec);
        client->set_read_timeout(timeout_sec);
        client->set_write_timeout(timeout_sec);

        // Note: SSL certificate verification should be enabled when OpenSSL is available
        // For now, we skip this to avoid compilation issues without OpenSSL
    }
};

HttpClient::HttpClient() : impl_(std::make_unique<Impl>()) {}

HttpClient::~HttpClient() = default;

HttpClient::HttpClient(HttpClient&&) noexcept = default;
HttpClient& HttpClient::operator=(HttpClient&&) noexcept = default;

void HttpClient::set_timeout(int seconds) {
    impl_->timeout_sec = seconds;
}

void HttpClient::set_header(const std::string& key, const std::string& value) {
    impl_->headers.insert({key, value});
}

HttpResponse HttpClient::get(const std::string& url) {
    impl_->update_client(url);

    auto parsed = impl_->parse_url(url);
    auto result = impl_->client->Get(parsed.path.c_str(), impl_->headers);

    if (!result) {
        throw NetworkError("HTTP GET failed: no response");
    }

    HttpResponse response;
    response.status_code = result->status;
    response.body = result->body;

    for (const auto& [key, value] : result->headers) {
        response.headers[key] = value;
    }

    return response;
}

HttpResponse HttpClient::post(const std::string& url, const std::string& body) {
    return post(url, body, "application/json");
}

HttpResponse HttpClient::post(const std::string& url,
                             const std::string& body,
                             const std::string& content_type) {
    impl_->update_client(url);

    auto parsed = impl_->parse_url(url);
    auto result = impl_->client->Post(parsed.path.c_str(),
                                      impl_->headers,
                                      body.c_str(),
                                      body.size(),
                                      content_type.c_str());

    if (!result) {
        throw NetworkError("HTTP POST failed: no response");
    }

    HttpResponse response;
    response.status_code = result->status;
    response.body = result->body;

    return response;
}

} // namespace boostchain
