// tests/unit/test_http_client.cpp
#include <boostchain/http_client.hpp>
#include <cassert>

void test_http_get() {
    boostchain::HttpClient client;
    auto response = client.get("https://httpbin.org/get");
    assert(response.status_code == 200);
    assert(!response.body.empty());
}

void test_http_post() {
    boostchain::HttpClient client;
    auto response = client.post("https://httpbin.org/post", R"({"test":"data"})");
    assert(response.status_code == 200);
}

void test_http_error() {
    boostchain::HttpClient client;
    bool threw = false;
    try {
        client.get("https://invalid-url-that-does-not-exist-12345.com");
    } catch (const boostchain::NetworkError& e) {
        threw = true;
    }
    assert(threw);
}

int main() {
    test_http_get();
    test_http_post();
    test_http_error();
    return 0;
}
