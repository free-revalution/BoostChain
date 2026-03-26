// src/http_client.cpp
#include <boostchain/http_client.hpp>
#include <httplib.h>
#include <memory>

namespace boostchain {

class HttpClient::Impl {
public:
    std::shared_ptr<httplib::Client> client;
    std::map<std::string, std::string> headers;
    int timeout_sec = 30;

    void update_client(const std::string& url) {
        // Parse URL to get host and port
        std::string host = url;
        std::string path = "/";

        size_t proto_pos = host.find("://");
        if (proto_pos != std::string::npos) {
            host = host.substr(proto_pos + 3);
        }

        size_t path_pos = host.find('/');
        if (path_pos != std::string::npos) {
            path = host.substr(path_pos);
            host = host.substr(0, path_pos);
        }

        bool https = url.substr(0, 5) == "https";

        client = std::make_shared<httplib::Client>(host.c_str());
        client->set_connection_timeout(timeout_sec);
        client->set_read_timeout(timeout_sec);
        client->set_write_timeout(timeout_sec);

        for (const auto& [key, value] : headers) {
            client->set_bearer_token_auth(key.c_str());  // Temporary
        }
    }
};

HttpClient::HttpClient() : impl_(new Impl()) {}

HttpClient::~HttpClient() = default;

HttpClient::HttpClient(HttpClient&&) noexcept = default;
HttpClient& HttpClient::operator=(HttpClient&&) noexcept = default;

void HttpClient::set_timeout(int seconds) {
    impl_->timeout_sec = seconds;
}

void HttpClient::set_header(const std::string& key, const std::string& value) {
    impl_->headers[key] = value;
}

HttpResponse HttpClient::get(const std::string& url) {
    impl_->update_client(url);

    std::string path = "/";
    size_t path_pos = url.find('/', url.find("://") + 3);
    if (path_pos != std::string::npos) {
        path = url.substr(path_pos);
    }

    auto result = impl_->client->Get(path.c_str());

    if (!result) {
        throw NetworkError("HTTP GET failed: no response");
    }

    HttpResponse response;
    response.status_code = result->status;
    response.body = result->body;

    for (const auto& [key, value] : result->headers) {
        response.headers[key] = value;
    }

    if (response.status_code >= 400) {
        throw NetworkError("HTTP GET failed with status " +
                          std::to_string(response.status_code),
                          response.status_code);
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

    std::string path = "/";
    size_t path_pos = url.find('/', url.find("://") + 3);
    if (path_pos != std::string::npos) {
        path = url.substr(path_pos);
    }

    auto result = impl_->client->Post(path.c_str(),
                                      body.c_str(),
                                      body.size(),
                                      content_type.c_str());

    if (!result) {
        throw NetworkError("HTTP POST failed: no response");
    }

    HttpResponse response;
    response.status_code = result->status;
    response.body = result->body;

    if (response.status_code >= 400) {
        throw NetworkError("HTTP POST failed with status " +
                          std::to_string(response.status_code),
                          response.status_code);
    }

    return response;
}

} // namespace boostchain
