# BoostChain Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build a lightweight, high-performance C++ Agent framework similar to LangChain, supporting universal LLM providers, chains, agents, memory, and tools.

**Architecture:** Four-layer architecture (User → Orchestration → Abstraction → Adapter) with interface-driven design, compile-time optimization, and static library deployment.

**Tech Stack:** C++17, CMake 3.15+, nlohmann/json (header-only), httplib (header-only), spdlog (optional)

---

## Phase 1: Project Setup & Core Infrastructure

### Task 1: Initialize Project Structure

**Files:**
- Create: `CMakeLists.txt`
- Create: `include/boostchain/boostchain.hpp`
- Create: `include/boostchain/config.hpp`
- Create: `include/boostchain/version.hpp`

**Step 1: Create root CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.15)
project(BoostChain VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BOOSTCHAIN_BUILD_TESTS "Build tests" ON)
option(BOOSTCHAIN_BUILD_EXAMPLES "Build examples" ON)
option(BOOSTCHAIN_DISABLE_EXCEPTIONS "Disable exceptions" OFF)

# Library target
add_library(boostchain)
add_library(boostchain::boostchain ALIAS boostchain)

target_include_directories(boostchain
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(boostchain PUBLIC cxx_std_17)

# Dependencies header-only
target_include_directories(boostchain PRIVATE deps)

# Subdirectories
if(BOOSTCHAIN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BOOSTCHAIN_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Install
install(TARGETS boostchain
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(DIRECTORY include/boostchain DESTINATION include)
```

**Step 2: Create version header**

```cpp
// include/boostchain/version.hpp
#pragma once

#define BOOSTCHAIN_VERSION_MAJOR 0
#define BOOSTCHAIN_VERSION_MINOR 1
#define BOOSTCHAIN_VERSION_PATCH 0

#define BOOSTCHAIN_VERSION "0.1.0"
```

**Step 3: Create config header**

```cpp
// include/boostchain/config.hpp
#pragma once

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
#define BOOSTCHAIN_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define BOOSTCHAIN_PLATFORM_MACOS
#elif defined(__linux__)
#define BOOSTCHAIN_PLATFORM_LINUX
#endif

// Compiler detection
#if defined(_MSC_VER)
#define BOOSTCHAIN_COMPILER_MSVC
#elif defined(__clang__)
#define BOOSTCHAIN_COMPILER_CLANG
#elif defined(__GNUC__)
#define BOOSTCHAIN_COMPILER_GCC
#endif

// Exception control
#ifdef BOOSTCHAIN_NO_EXCEPTIONS
#define BOOSTCHAIN_THROW(msg) std::abort()
#else
#define BOOSTCHAIN_THROW(msg) throw(msg)
#endif
```

**Step 4: Create main header**

```cpp
// include/boostchain/boostchain.hpp
#pragma once

// Version
#include <boostchain/version.hpp>

// Core components
#include <boostchain/config.hpp>

// Version info
namespace boostchain {
    inline constexpr int version_major() { return BOOSTCHAIN_VERSION_MAJOR; }
    inline constexpr int version_minor() { return BOOSTCHAIN_VERSION_MINOR; }
    inline constexpr int version_patch() { return BOOSTCHAIN_VERSION_PATCH; }
    inline constexpr const char* version() { return BOOSTCHAIN_VERSION; }
}
```

**Step 5: Create tests directory structure**

```bash
mkdir -p tests/unit tests/integration tests/mock
touch tests/CMakeLists.txt
```

**Step 6: Create tests/CMakeLists.txt**

```cmake
# Simple test framework setup
add_subdirectory(unit)
add_subdirectory(integration)
```

**Step 7: Commit**

```bash
git add CMakeLists.txt include/boostchain/ tests/
git commit -m "feat: initialize project structure and build system"
```

---

### Task 2: Error Handling System

**Files:**
- Create: `include/boostchain/error.hpp`
- Create: `src/error.cpp`
- Test: `tests/unit/test_error.cpp`

**Step 1: Write failing tests**

```cpp
// tests/unit/test_error.cpp
#include <boostchain/error.hpp>
#include <cassert>

void test_error_hierarchy() {
    try {
        throw boostchain::NetworkError("Connection failed", 500);
    } catch (const boostchain::Error& e) {
        assert(std::string(e.what()) == "Connection failed");
    }
}

void test_network_error_status_code() {
    boostchain::NetworkError err("Timeout", 408);
    assert(err.status_code() == 408);
}

void test_tool_error_name() {
    boostchain::ToolError err("calculator", "Division by zero");
    assert(err.tool_name() == "calculator");
    assert(std::string(err.what()).find("calculator") != std::string::npos);
}

int main() {
    test_error_hierarchy();
    test_network_error_status_code();
    test_tool_error_name();
    return 0;
}
```

**Step 2: Run test to verify it fails**

```bash
cd build && cmake .. && make
./tests/unit/test_error || true  # Should fail - file not exists
```

Expected: Compilation error - error.hpp not found

**Step 3: Write error.hpp**

```cpp
// include/boostchain/error.hpp
#pragma once

#include <stdexcept>
#include <string>

namespace boostchain {

// Base error class
class Error : public std::runtime_error {
public:
    explicit Error(const std::string& msg) : std::runtime_error(msg) {}
    virtual ~Error() = default;
};

// Network error with status code
class NetworkError : public Error {
public:
    explicit NetworkError(const std::string& msg, int status_code = 0)
        : Error(msg), status_code_(status_code) {}

    int status_code() const { return status_code_; }

private:
    int status_code_;
};

// API error from LLM provider
class APIError : public Error {
public:
    explicit APIError(const std::string& msg,
                     const std::string& raw_response = "")
        : Error(msg), raw_response_(raw_response) {}

    const std::string& raw_response() const { return raw_response_; }

private:
    std::string raw_response_;
};

// Tool execution error
class ToolError : public Error {
public:
    explicit ToolError(const std::string& tool_name,
                      const std::string& reason)
        : Error(tool_name + ": " + reason), tool_name_(tool_name) {}

    const std::string& tool_name() const { return tool_name_; }

private:
    std::string tool_name_;
};

// Serialization error
class SerializationError : public Error {
public:
    explicit SerializationError(const std::string& msg)
        : Error(msg) {}
};

// Configuration error
class ConfigError : public Error {
public:
    explicit ConfigError(const std::string& msg)
        : Error(msg) {}
};

} // namespace boostchain
```

**Step 4: Update CMakeLists.txt to add error.cpp**

```cmake
# Add to target_sources(boostchain ...)
target_sources(boostchain
    PRIVATE
        src/error.cpp
)
```

**Step 5: Create minimal error.cpp**

```cpp
// src/error.cpp
#include <boostchain/error.hpp>

// Empty - all inline definitions
```

**Step 6: Create tests/unit/CMakeLists.txt**

```cmake
add_executable(test_error test_error.cpp)
target_link_libraries(test_error boostchain::boostchain)

add_test(NAME test_error COMMAND test_error)
```

**Step 7: Run test to verify it passes**

```bash
cd build && make test_error
./tests/unit/test_error
```

Expected: All tests PASS

**Step 8: Commit**

```bash
git add include/boostchain/error.hpp src/error.cpp tests/unit/test_error.cpp tests/unit/CMakeLists.txt
git commit -m "feat: implement error handling system"
```

---

### Task 3: Result<T> Type (No-Exception Path)

**Files:**
- Create: `include/boostchain/result.hpp`
- Test: `tests/unit/test_result.cpp`

**Step 1: Write failing tests**

```cpp
// tests/unit/test_result.cpp
#include <boostchain/result.hpp>
#include <cassert>
#include <string>

void test_result_ok() {
    boostchain::Result<int> r = boostchain::Result<int>::ok(42);
    assert(r.is_ok() == true);
    assert(r.is_error() == false);
    assert(r.unwrap() == 42);
}

void test_result_error() {
    auto r = boostchain::Result<int>::error("Something failed");
    assert(r.is_ok() == false);
    assert(r.is_error() == true);
    assert(r.error_msg() == "Something failed");
}

void test_result_map() {
    auto r = boostchain::Result<int>::ok(5);
    auto mapped = r.map([](int x) { return x * 2; });
    assert(mapped.is_ok());
    assert(mapped.unwrap() == 10);
}

void test_result_map_error_propagates() {
    auto r = boostchain::Result<int>::error("bad");
    auto mapped = r.map<int>([](int x) { return x * 2; });
    assert(mapped.is_error());
    assert(mapped.error_msg() == "bad");
}

void test_result_string() {
    auto r = boostchain::Result<std::string>::ok(std::string("hello"));
    assert(r.is_ok());
    assert(r.unwrap() == "hello");
}

int main() {
    test_result_ok();
    test_result_error();
    test_result_map();
    test_result_map_error_propagates();
    test_result_string();
    return 0;
}
```

**Step 2: Run test to verify it fails**

Expected: Compilation error - result.hpp not found

**Step 3: Write result.hpp**

```cpp
// include/boostchain/result.hpp
#pragma once

#include <optional>
#include <string>
#include <stdexcept>
#include <utility>

namespace boostchain {

template<typename T>
class Result {
public:
    // Success constructor
    static Result ok(T value) {
        return Result(std::move(value), std::nullopt);
    }

    // Error constructor
    static Result error(std::string message) {
        return Result(std::nullopt, std::move(message));
    }

    // Query state
    bool is_ok() const { return value_.has_value(); }
    bool is_error() const { return error_.has_value(); }

    // Get value (throws if error)
    T& unwrap() {
        if (is_error()) {
            throw std::runtime_error(error_.value());
        }
        return value_.value();
    }

    const T& unwrap() const {
        if (is_error()) {
            throw std::runtime_error(error_.value());
        }
        return value_.value();
    }

    // Get error message
    const std::string& error_msg() const {
        static const std::string empty;
        return error_.value_or(empty);
    }

    // Map operation (functional chaining)
    template<typename F>
    auto map(F&& func) -> Result<decltype(func(std::declval<T&>()))> {
        using U = decltype(func(std::declval<T&>()));
        if (is_error()) {
            return Result<U>::error(error_msg());
        }
        try {
            return Result<U>::ok(func(value_.value()));
        } catch (const std::exception& e) {
            return Result<U>::error(std::string("map failed: ") + e.what());
        }
    }

private:
    Result(std::optional<T> value, std::optional<std::string> error)
        : value_(std::move(value)), error_(std::move(error)) {}

    std::optional<T> value_;
    std::optional<std::string> error_;
};

} // namespace boostchain
```

**Step 4: Update tests/unit/CMakeLists.txt**

```cmake
add_executable(test_result test_result.cpp)
target_link_libraries(test_result boostchain::boostchain)
add_test(NAME test_result COMMAND test_result)
```

**Step 5: Run test to verify it passes**

```bash
cd build && make test_result
./tests/unit/test_result
```

Expected: All tests PASS

**Step 6: Commit**

```bash
git add include/boostchain/result.hpp tests/unit/test_result.cpp
git commit -m "feat: implement Result<T> for no-exception error handling"
```

---

### Task 4: HTTP Client Wrapper

**Files:**
- Create: `deps/` (download dependencies)
- Create: `include/boostchain/http_client.hpp`
- Create: `src/http_client.cpp`
- Test: `tests/unit/test_http_client.cpp`

**Step 1: Download header-only dependencies**

```bash
cd deps
curl -o json.hpp https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp
curl -o httplib.h https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h
```

**Step 2: Write failing tests**

```cpp
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
```

**Step 3: Run test to verify it fails**

Expected: Compilation error - http_client.hpp not found

**Step 4: Write http_client.hpp**

```cpp
// include/boostchain/http_client.hpp
#pragma once

#include <boostchain/error.hpp>
#include <string>
#include <map>

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
    Impl* impl_;
};

} // namespace boostchain
```

**Step 5: Write http_client.cpp**

```cpp
// src/http_client.cpp
#include <boostchain/http_client.hpp>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <sstream>

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
```

**Step 6: Update CMakeLists.txt**

```cmake
target_sources(boostchain
    PRIVATE
        src/http_client.cpp
)
```

**Step 7: Update tests/unit/CMakeLists.txt**

```cmake
add_executable(test_http_client test_http_client.cpp)
target_link_libraries(test_http_client boostchain::boostchain)
add_test(NAME test_http_client COMMAND test_http_client)
```

**Step 8: Run test to verify it passes**

```bash
cd build && make test_http_client
./tests/unit/test_http_client
```

Expected: All tests PASS (requires internet)

**Step 9: Commit**

```bash
git add deps/ include/boostchain/http_client.hpp src/http_client.cpp tests/unit/test_http_client.cpp
git commit -m "feat: implement HTTP client wrapper with httplib"
```

---

## Phase 2: LLM Provider Layer

### Task 5: LLM Provider Interface

**Files:**
- Create: `include/boostchain/llm_provider.hpp`
- Test: `tests/unit/test_llm_provider_interface.cpp`

**Step 1: Write failing tests**

```cpp
// tests/unit/test_llm_provider_interface.cpp
#include <boostchain/llm_provider.hpp>
#include <cassert>
#include <memory>

using namespace boostchain;

void test_message_construction() {
    Message msg;
    msg.role = Message::user;
    msg.content = "Hello";

    assert(msg.role == Message::user);
    assert(msg.content == "Hello");
    assert(!msg.tool_call.has_value());
}

void test_tool_call_structure() {
    ToolCall call;
    call.id = "call_123";
    call.name = "calculator";
    call.arguments = R"({"expression":"2+2"})";

    assert(call.name == "calculator");
    assert(call.arguments.find("2+2") != std::string::npos);
}

void test_chat_request_structure() {
    ChatRequest req;
    req.model = "gpt-4";
    req.temperature = 0.7;
    req.messages.push_back({Message::user, "Hello"});

    assert(req.model == "gpt-4");
    assert(req.messages.size() == 1);
}

void test_tool_definition() {
    ToolDefinition tool;
    tool.name = "http_request";
    tool.description = "Make HTTP requests";
    tool.parameters["url"] = ToolParameter{"string", "Request URL", true};

    assert(tool.name == "http_request");
    assert(tool.parameters.at("url").required == true);
}

int main() {
    test_message_construction();
    test_tool_call_structure();
    test_chat_request_structure();
    test_tool_definition();
    return 0;
}
```

**Step 2: Run test to verify it fails**

Expected: Compilation error - llm_provider.hpp not found

**Step 3: Write llm_provider.hpp**

```cpp
// include/boostchain/llm_provider.hpp
#pragma once

#include <boostchain/result.hpp>
#include <boostchain/error.hpp>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <functional>
#include <future>

namespace boostchain {

// Message role and content
struct Message {
    enum Role { system, user, assistant, tool };
    Role role;
    std::string content;

    // For tool-related messages
    std::optional<std::string> tool_call_id;
    std::optional<struct ToolCall> tool_call;
};

// Tool call from LLM
struct ToolCall {
    std::string id;
    std::string name;
    std::string arguments;  // JSON string
};

// Tool parameter definition
struct ToolParameter {
    std::string type;
    std::string description;
    bool required = false;
    std::optional<std::string> default_value;
};

// Tool definition for LLM
struct ToolDefinition {
    std::string name;
    std::string description;
    std::map<std::string, ToolParameter> parameters;
};

// Chat request
struct ChatRequest {
    std::vector<Message> messages;
    std::string model;
    double temperature = 0.7;
    int max_tokens = 1024;
    std::vector<ToolDefinition> tools;
    bool stream = false;
};

// Chat response
struct ChatResponse {
    Message message;
    std::optional<ToolCall> tool_call;
    std::map<std::string, std::string> usage;  // token counts
    std::string raw_response;
    std::string model;
};

// Embedding response
struct EmbeddingResponse {
    std::vector<float> embedding;
    std::string model;
    int tokens_used;
};

// Stream callback type
using StreamCallback = std::function<void(const std::string&)>;

// Abstract LLM provider interface
class ILLMProvider {
public:
    virtual ~ILLMProvider() = default;

    // Synchronous chat
    virtual ChatResponse chat(const ChatRequest& req) = 0;

    // Asynchronous chat
    virtual std::future<ChatResponse> chat_async(const ChatRequest& req) = 0;

    // Streaming chat
    virtual void chat_stream(const ChatRequest& req,
                            StreamCallback callback) = 0;

    // Embedding (optional, default throws)
    virtual std::vector<float> embed(const std::string& text) {
        throw Error("Embedding not supported by this provider");
    }

    // Safe versions (no exceptions)
    virtual Result<ChatResponse> chat_safe(const ChatRequest& req) noexcept {
        try {
            return Result<ChatResponse>::ok(chat(req));
        } catch (const NetworkError& e) {
            return Result<ChatResponse>::error(std::string("Network: ") + e.what());
        } catch (const APIError& e) {
            return Result<ChatResponse>::error(std::string("API: ") + e.what());
        } catch (...) {
            return Result<ChatResponse>::error("Unknown error");
        }
    }

    // Set API key
    virtual void set_api_key(const std::string& key) {
        api_key_ = key;
    }

    // Set model
    virtual void set_model(const std::string& model) {
        default_model_ = model;
    }

    // Set base URL
    virtual void set_base_url(const std::string& url) {
        base_url_ = url;
    }

protected:
    std::string api_key_;
    std::string default_model_;
    std::string base_url_;
};

} // namespace boostchain
```

**Step 4: Update tests/unit/CMakeLists.txt**

```cmake
add_executable(test_llm_provider_interface test_llm_provider_interface.cpp)
target_link_libraries(test_llm_provider_interface boostchain::boostchain)
add_test(NAME test_llm_provider_interface COMMAND test_llm_provider_interface)
```

**Step 5: Run test to verify it passes**

```bash
cd build && make test_llm_provider_interface
./tests/unit/test_llm_provider_interface
```

Expected: All tests PASS

**Step 6: Commit**

```bash
git add include/boostchain/llm_provider.hpp tests/unit/test_llm_provider_interface.cpp
git commit -m "feat: define LLM provider abstract interface"
```

---

### Task 6: OpenAI Provider Implementation

**Files:**
- Create: `include/boostchain/llm/openai_provider.hpp`
- Create: `src/llm/openai_provider.cpp`
- Test: `tests/integration/test_openai_provider.cpp`

**Step 1: Write failing tests**

```cpp
// tests/integration/test_openai_provider.cpp
#include <boostchain/llm/openai_provider.hpp>
#include <cassert>
#include <iostream>

using namespace boostchain;

int main() {
    // Load API key from environment
    const char* api_key = std::getenv("OPENAI_API_KEY");
    if (!api_key) {
        std::cout << "SKIP: OPENAI_API_KEY not set\n";
        return 0;
    }

    OpenAIProvider provider(api_key);
    provider.set_model("gpt-3.5-turbo");

    // Test basic chat
    ChatRequest req;
    req.messages = {{Message::user, "Say 'Hello, BoostChain!'"}};

    auto response = provider.chat(req);

    assert(!response.message.content.empty());
    assert(response.model == "gpt-3.5-turbo");

    std::cout << "Response: " << response.message.content << "\n";

    return 0;
}
```

**Step 2: Run test to verify it fails**

Expected: Compilation error - openai_provider.hpp not found

**Step 3: Write openai_provider.hpp**

```cpp
// include/boostchain/llm/openai_provider.hpp
#pragma once

#include <boostchain/llm_provider.hpp>
#include <boostchain/http_client.hpp>
#include <string>

namespace boostchain {

class OpenAIProvider : public ILLMProvider {
public:
    explicit OpenAIProvider(const std::string& api_key,
                           const std::string& base_url = "https://api.openai.com/v1");

    ChatResponse chat(const ChatRequest& req) override;
    std::future<ChatResponse> chat_async(const ChatRequest& req) override;
    void chat_stream(const ChatRequest& req,
                    StreamCallback callback) override;

private:
    HttpClient http_client_;
    std::string format_messages(const std::vector<Message>& messages);
    std::string format_tools(const std::vector<ToolDefinition>& tools);
    ChatResponse parse_response(const std::string& json_str);
};

} // namespace boostchain
```

**Step 4: Write openai_provider.cpp**

```cpp
// src/llm/openai_provider.cpp
#include <boostchain/llm/openai_provider.hpp>
#include <nlohmann/json.hpp>
#include <future>
#include <thread>

namespace boostchain {

OpenAIProvider::OpenAIProvider(const std::string& api_key,
                               const std::string& base_url)
    : http_client_() {
    api_key_ = api_key;
    base_url_ = base_url;
    http_client_.set_header("Authorization", "Bearer " + api_key);
    http_client_.set_timeout(60);
}

std::string OpenAIProvider::format_messages(const std::vector<Message>& messages) {
    nlohmann::json msg_array = nlohmann::json::array();
    for (const auto& msg : messages) {
        nlohmann::json j;
        switch (msg.role) {
            case Message::system: j["role"] = "system"; break;
            case Message::user: j["role"] = "user"; break;
            case Message::assistant: j["role"] = "assistant"; break;
            case Message::tool: j["role"] = "tool"; break;
        }
        j["content"] = msg.content;
        if (msg.tool_call_id) {
            j["tool_call_id"] = *msg.tool_call_id;
        }
        msg_array.push_back(j);
    }
    return msg_array.dump();
}

std::string OpenAIProvider::format_tools(const std::vector<ToolDefinition>& tools) {
    nlohmann::json tools_array = nlohmann::json::array();
    for (const auto& tool : tools) {
        nlohmann::json j;
        j["type"] = "function";
        j["function"]["name"] = tool.name;
        j["function"]["description"] = tool.description;

        nlohmann::json params;
        params["type"] = "object";
        nlohmann::json properties;
        for (const auto& [name, param] : tool.parameters) {
            nlohmann::json p;
            p["type"] = param.type;
            p["description"] = param.description;
            if (param.default_value) {
                p["default"] = *param.default_value;
            }
            properties[name] = p;
        }
        params["properties"] = properties;

        std::vector<std::string> required;
        for (const auto& [name, param] : tool.parameters) {
            if (param.required) {
                required.push_back(name);
            }
        }
        if (!required.empty()) {
            params["required"] = required;
        }

        j["function"]["parameters"] = params;
        tools_array.push_back(j);
    }
    return tools_array.dump();
}

ChatResponse OpenAIProvider::chat(const ChatRequest& req) {
    nlohmann::json j;
    j["model"] = req.model.empty() ? default_model_ : req.model;
    j["messages"] = nlohmann::json::parse(format_messages(req.messages));
    j["temperature"] = req.temperature;
    j["max_tokens"] = req.max_tokens;

    if (!req.tools.empty()) {
        j["tools"] = nlohmann::json::parse(format_tools(req.tools));
    }

    std::string url = base_url_ + "/chat/completions";
    auto http_resp = http_client_.post(url, j.dump());

    return parse_response(http_resp.body);
}

std::future<ChatResponse> OpenAIProvider::chat_async(const ChatRequest& req) {
    return std::async(std::launch::async, [this, req]() {
        return chat(req);
    });
}

void OpenAIProvider::chat_stream(const ChatRequest& req,
                                StreamCallback callback) {
    // For simplicity, non-streaming implementation
    // TODO: Implement actual SSE streaming
    auto response = chat(req);
    callback(response.message.content);
}

ChatResponse OpenAIProvider::parse_response(const std::string& json_str) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_str);

        ChatResponse resp;
        resp.raw_response = json_str;

        if (j.contains("model")) {
            resp.model = j["model"];
        }

        if (j.contains("usage")) {
            auto& usage = j["usage"];
            if (usage.contains("prompt_tokens")) {
                resp.usage["prompt_tokens"] = std::to_string(usage["prompt_tokens"].get<int>());
            }
            if (usage.contains("completion_tokens")) {
                resp.usage["completion_tokens"] = std::to_string(usage["completion_tokens"].get<int>());
            }
        }

        if (j.contains("choices") && !j["choices"].empty()) {
            auto& choice = j["choices"][0];
            if (choice.contains("message")) {
                auto& msg = choice["message"];

                resp.message.role = Message::assistant;
                if (msg.contains("content")) {
                    resp.message.content = msg["content"].get<std::string>();
                }

                // Check for tool calls
                if (msg.contains("tool_calls") && !msg["tool_calls"].empty()) {
                    auto& tool_call = msg["tool_calls"][0];
                    ToolCall tc;
                    if (tool_call.contains("id")) {
                        tc.id = tool_call["id"];
                    }
                    if (tool_call.contains("function")) {
                        auto& func = tool_call["function"];
                        if (func.contains("name")) {
                            tc.name = func["name"];
                        }
                        if (func.contains("arguments")) {
                            tc.arguments = func["arguments"];
                        }
                    }
                    resp.tool_call = tc;
                }
            }
        }

        return resp;

    } catch (const std::exception& e) {
        throw APIError("Failed to parse OpenAI response: " + std::string(e.what()),
                      json_str);
    }
}

} // namespace boostchain
```

**Step 5: Update CMakeLists.txt**

```cmake
target_sources(boostchain
    PRIVATE
        src/llm/openai_provider.cpp
)

target_include_directories(boostchain
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
```

**Step 6: Create tests/integration/CMakeLists.txt**

```cmake
add_executable(test_openai_provider test_openai_provider.cpp)
target_link_libraries(test_openai_provider boostchain::boostchain)
add_test(NAME test_openai_provider COMMAND test_openai_provider)
```

**Step 7: Run test (requires API key)**

```bash
export OPENAI_API_KEY="your-key-here"
cd build && make test_openai_provider
./tests/integration/test_openai_provider
```

Expected: PASS if API key is valid

**Step 8: Commit**

```bash
git add include/boostchain/llm/ src/llm/ tests/integration/
git commit -m "feat: implement OpenAI provider with chat support"
```

---

### Task 7: OpenAI-Compatible Provider (Universal Support)

**Files:**
- Create: `include/boostchain/llm/openai_compatible.hpp`
- Create: `src/llm/openai_compatible.cpp`
- Test: `tests/integration/test_openai_compatible.cpp`

**Step 1: Write failing tests**

```cpp
// tests/integration/test_openai_compatible.cpp
#include <boostchain/llm/openai_compatible.hpp>
#include <cassert>
#include <iostream>

using namespace boostchain;

int main() {
    // Test with DeepSeek (or any OpenAI-compatible API)
    const char* api_key = std::getenv("DEEPSEEK_API_KEY");
    if (!api_key) {
        std::cout << "SKIP: DEEPSEEK_API_KEY not set\n";
        return 0;
    }

    auto provider = std::make_shared<OpenAICompatibleProvider>(
        "https://api.deepseek.com/v1",
        api_key
    );
    provider->set_model("deepseek-chat");

    ChatRequest req;
    req.messages = {{Message::user, "Hello! What is 2+2?"}};

    auto response = provider->chat(req);

    assert(!response.message.content.empty());
    std::cout << "DeepSeek: " << response.message.content << "\n";

    return 0;
}
```

**Step 2: Run test to verify it fails**

Expected: Compilation error - openai_compatible.hpp not found

**Step 3: Write openai_compatible.hpp**

```cpp
// include/boostchain/llm/openai_compatible.hpp
#pragma once

#include <boostchain/llm_provider.hpp>
#include <string>

namespace boostchain {

// Universal adapter for OpenAI-compatible APIs
// Supports: DeepSeek, Kimi, GLM, Qwen, and many others
class OpenAICompatibleProvider : public ILLMProvider {
public:
    OpenAICompatibleProvider(const std::string& base_url,
                            const std::string& api_key);

    ChatResponse chat(const ChatRequest& req) override;
    std::future<ChatResponse> chat_async(const ChatRequest& req) override;
    void chat_stream(const ChatRequest& req,
                    StreamCallback callback) override;

private:
    // Reuse OpenAI implementation internally
    class OpenAIProvider impl_;
};

} // namespace boostchain
```

**Step 4: Write openai_compatible.cpp**

```cpp
// src/llm/openai_compatible.cpp
#include <boostchain/llm/openai_compatible.hpp>
#include <boostchain/llm/openai_provider.hpp>

namespace boostchain {

// Private implementation using OpenAI provider
class OpenAICompatibleProvider::OpenAIProvider {
public:
    ::boostchain::OpenAIProvider provider;

    OpenAIProvider(const std::string& base_url, const std::string& api_key)
        : provider(api_key, base_url) {}
};

OpenAICompatibleProvider::OpenAICompatibleProvider(
    const std::string& base_url,
    const std::string& api_key)
    : impl_(*new OpenAIProvider(base_url, api_key)) {}

ChatResponse OpenAICompatibleProvider::chat(const ChatRequest& req) {
    return impl_.provider.chat(req);
}

std::future<ChatResponse> OpenAICompatibleProvider::chat_async(
    const ChatRequest& req) {
    return impl_.provider.chat_async(req);
}

void OpenAICompatibleProvider::chat_stream(
    const ChatRequest& req,
    StreamCallback callback) {
    impl_.provider.chat_stream(req, callback);
}

} // namespace boostchain
```

**Step 5: Update CMakeLists.txt**

```cmake
target_sources(boostchain
    PRIVATE
        src/llm/openai_compatible.cpp
)
```

**Step 6: Update tests/integration/CMakeLists.txt**

```cmake
add_executable(test_openai_compatible test_openai_compatible.cpp)
target_link_libraries(test_openai_compatible boostchain::boostchain)
add_test(NAME test_openai_compatible COMMAND test_openai_compatible)
```

**Step 7: Run test**

```bash
export DEEPSEEK_API_KEY="your-key"
cd build && make test_openai_compatible
./tests/integration/test_openai_compatible
```

**Step 8: Commit**

```bash
git add include/boostchain/llm/openai_compatible.hpp src/llm/openai_compatible.cpp tests/integration/test_openai_compatible.cpp
git commit -m "feat: add OpenAI-compatible provider for universal LLM support"
```

---

## Phase 3: Chain Engine

### Task 8: Prompt Template System

**Files:**
- Create: `include/boostchain/prompt.hpp`
- Create: `src/prompt.cpp`
- Test: `tests/unit/test_prompt.cpp`

**Step 1: Write failing tests**

```cpp
// tests/unit/test_prompt.cpp
#include <boostchain/prompt.hpp>
#include <cassert>
#include <map>

using namespace boostchain;

void test_simple_template() {
    PromptTemplate tpl("Hello, {{name}}!");
    Variables vars;
    vars["name"] = "World";

    std::string result = tpl.render(vars);
    assert(result == "Hello, World!");
}

void test_multiple_variables() {
    PromptTemplate tpl("{{greeting}}, {{name}}! Today is {{day}}.");
    Variables vars;
    vars["greeting"] = "Hello";
    vars["name"] = "Alice";
    vars["day"] = "Monday";

    std::string result = tpl.render(vars);
    assert(result == "Hello, Alice! Today is Monday.");
}

void test_missing_variable() {
    PromptTemplate tpl("Hello, {{name}}!");
    Variables vars;  // name not provided

    bool threw = false;
    try {
        tpl.render(vars);
    } catch (const ConfigError& e) {
        threw = true;
    }
    assert(threw);
}

void test_no_variables() {
    PromptTemplate tpl("Just static text");
    Variables vars;

    std::string result = tpl.render(vars);
    assert(result == "Just static text");
}

int main() {
    test_simple_template();
    test_multiple_variables();
    test_missing_variable();
    test_no_variables();
    return 0;
}
```

**Step 2: Run test to verify it fails**

Expected: Compilation error - prompt.hpp not found

**Step 3: Write prompt.hpp**

```cpp
// include/boostchain/prompt.hpp
#pragma once

#include <boostchain/error.hpp>
#include <string>
#include <map>
#include <regex>

namespace boostchain {

// Variables for template rendering
using Variables = std::map<std::string, std::string>;

// Prompt template with variable substitution
class PromptTemplate {
public:
    explicit PromptTemplate(const std::string& template_str);

    // Render template with variables
    std::string render(const Variables& vars) const;

    // Get original template
    const std::string& template_str() const { return template_; }

    // Extract variable names from template
    std::vector<std::string> variables() const;

private:
    std::string template_;
    static const std::regex variable_regex_;
};

} // namespace boostchain
```

**Step 4: Write prompt.cpp**

```cpp
// src/prompt.cpp
#include <boostchain/prompt.hpp>
#include <sstream>

namespace boostchain {

const std::regex PromptTemplate::variable_regex_(R"(\{\{(\w+)\}\})");

PromptTemplate::PromptTemplate(const std::string& template_str)
    : template_(template_str) {}

std::string PromptTemplate::render(const Variables& vars) const {
    std::string result = template_;
    std::smatch match;

    // Check for missing variables
    auto var_names = variables();
    for (const auto& name : var_names) {
        if (vars.find(name) == vars.end()) {
            throw ConfigError("Missing variable: " + name);
        }
    }

    // Replace all variables
    size_t pos = 0;
    while ((pos = result.find("{{", pos)) != std::string::npos) {
        size_t end = result.find("}}", pos);
        if (end == std::string::npos) break;

        std::string var_name = result.substr(pos + 2, end - pos - 2);
        auto it = vars.find(var_name);
        if (it != vars.end()) {
            result.replace(pos, end - pos + 2, it->second);
            pos += it->second.length();
        } else {
            pos = end + 2;
        }
    }

    return result;
}

std::vector<std::string> PromptTemplate::variables() const {
    std::vector<std::string> vars;
    std::set<std::string> unique_vars;

    auto words_begin = std::sregex_iterator(
        template_.begin(), template_.end(), variable_regex_
    );
    auto words_end = std::sregex_iterator();

    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        if (match.size() > 1) {
            unique_vars.insert(match[1].str());
        }
    }

    for (const auto& var : unique_vars) {
        vars.push_back(var);
    }

    return vars;
}

} // namespace boostchain
```

**Step 5: Update CMakeLists.txt**

```cmake
target_sources(boostchain
    PRIVATE
        src/prompt.cpp
)
```

**Step 6: Update tests/unit/CMakeLists.txt**

```cmake
add_executable(test_prompt test_prompt.cpp)
target_link_libraries(test_prompt boostchain::boostchain)
add_test(NAME test_prompt COMMAND test_prompt)
```

**Step 7: Run test to verify it passes**

```bash
cd build && make test_prompt
./tests/unit/test_prompt
```

**Step 8: Commit**

```bash
git add include/boostchain/prompt.hpp src/prompt.cpp tests/unit/test_prompt.cpp
git commit -m "feat: implement prompt template system with variable substitution"
```

---

### Task 9: Chain Engine Core

**Files:**
- Create: `include/boostchain/chain.hpp`
- Create: `src/chain.cpp`
- Test: `tests/unit/test_chain.cpp`

**Step 1: Write failing tests**

```cpp
// tests/unit/test_chain.cpp
#include <boostchain/chain.hpp>
#include <boostchain/llm/openai_provider.hpp>
#include <cassert>

using namespace boostchain;

int main() {
    // Create a simple chain: prompt -> LLM
    Chain chain;

    // Note: This test uses mock, actual LLM tests in integration

    // Test chain construction
    chain.add_prompt("You are a helpful assistant.");

    // Test serialization
    std::string serialized = chain.serialize();
    assert(!serialized.empty());

    Chain deserialized = Chain::deserialize(serialized);

    return 0;
}
```

**Step 2: Run test to verify it fails**

Expected: Compilation error - chain.hpp not found

**Step 3: Write chain.hpp**

```cpp
// include/boostchain/chain.hpp
#pragma once

#include <boostchain/llm_provider.hpp>
#include <boostchain/prompt.hpp>
#include <boostchain/result.hpp>
#include <memory>
#include <vector>
#include <string>

namespace boostchain {

// Forward declarations
class INode;

// Chain execution result
struct ChainResult {
    Variables outputs;
    std::string final_output;
    std::map<std::string, std::string> metadata;
};

// Chain - sequence of operations
class Chain {
public:
    Chain();
    ~Chain();

    // Add nodes to chain
    Chain& add_prompt(const std::string& template_str);
    Chain& add_llm(std::shared_ptr<ILLMProvider> provider);
    Chain& add_output_parser(const std::string& format);

    // Execute chain
    ChainResult run(const Variables& input = {});
    std::future<ChainResult> run_async(const Variables& input = {});

    // Serialization
    std::string serialize() const;
    static Chain deserialize(const std::string& data);

private:
    std::vector<std::unique_ptr<INode>> nodes_;
};

} // namespace boostchain
```

**Step 4: Write chain.cpp**

```cpp
// src/chain.cpp
#include <boostchain/chain.hpp>
#include <nlohmann/json.hpp>

namespace boostchain {

// Node interface (internal)
class INode {
public:
    virtual ~INode() = default;
    virtual Variables process(const Variables& input) = 0;
    virtual std::string serialize() const = 0;
};

// Prompt template node
class PromptNode : public INode {
public:
    explicit PromptNode(const std::string& template_str)
        : template_(template_str) {}

    Variables process(const Variables& input) override {
        Variables result = input;
        result["_prompt"] = template_.render(input);
        return result;
    }

    std::string serialize() const override {
        nlohmann::json j;
        j["type"] = "prompt";
        j["template"] = template_.template_str();
        return j.dump();
    }

private:
    PromptTemplate template_;
};

// LLM node
class LLMNode : public INode {
public:
    explicit LLMNode(std::shared_ptr<ILLMProvider> provider)
        : provider_(std::move(provider)) {}

    Variables process(const Variables& input) override {
        ChatRequest req;

        // Build messages from input
        if (input.find("_prompt") != input.end()) {
            req.messages.push_back({Message::system, input.at("_prompt")});
        }

        if (input.find("user_input") != input.end()) {
            req.messages.push_back({Message::user, input.at("user_input")});
        }

        auto response = provider_->chat(req);

        Variables result = input;
        result["_output"] = response.message.content;
        result["output"] = response.message.content;
        return result;
    }

    std::string serialize() const override {
        nlohmann::json j;
        j["type"] = "llm";
        // Note: provider reference not serialized
        return j.dump();
    }

private:
    std::shared_ptr<ILLMProvider> provider_;
};

// Chain implementation
Chain::Chain() = default;
Chain::~Chain() = default;

Chain& Chain::add_prompt(const std::string& template_str) {
    nodes_.push_back(std::make_unique<PromptNode>(template_str));
    return *this;
}

Chain& Chain::add_llm(std::shared_ptr<ILLMProvider> provider) {
    nodes_.push_back(std::make_unique<LLMNode>(provider));
    return *this;
}

ChainResult Chain::run(const Variables& input) {
    Variables current = input;

    for (auto& node : nodes_) {
        current = node->process(current);
    }

    ChainResult result;
    result.outputs = current;

    auto it = current.find("output");
    if (it != current.end()) {
        result.final_output = it->second;
    }

    return result;
}

std::future<ChainResult> Chain::run_async(const Variables& input) {
    return std::async(std::launch::async, [this, input]() {
        return run(input);
    });
}

std::string Chain::serialize() const {
    nlohmann::json j;
    j["nodes"] = nlohmann::json::array();

    for (const auto& node : nodes_) {
        j["nodes"].push_back(nlohmann::json::parse(node->serialize()));
    }

    return j.dump();
}

Chain Chain::deserialize(const std::string& data) {
    Chain chain;

    try {
        nlohmann::json j = nlohmann::json::parse(data);

        if (j.contains("nodes")) {
            for (const auto& node_json : j["nodes"]) {
                std::string type = node_json["type"];
                if (type == "prompt") {
                    chain.add_prompt(node_json["template"]);
                }
                // Note: LLM nodes need provider re-attachment
            }
        }
    } catch (const std::exception& e) {
        throw SerializationError("Failed to deserialize chain: " + std::string(e.what()));
    }

    return chain;
}

} // namespace boostchain
```

**Step 5: Update CMakeLists.txt**

```cmake
target_sources(boostchain
    PRIVATE
        src/chain.cpp
)
```

**Step 6: Update tests/unit/CMakeLists.txt**

```cmake
add_executable(test_chain test_chain.cpp)
target_link_libraries(test_chain boostchain::boostchain)
add_test(NAME test_chain COMMAND test_chain)
```

**Step 7: Run test to verify it passes**

```bash
cd build && make test_chain
./tests/unit/test_chain
```

**Step 8: Commit**

```bash
git add include/boostchain/chain.hpp src/chain.cpp tests/unit/test_chain.cpp
git commit -m "feat: implement Chain engine with prompt and LLM nodes"
```

---

### Task 10: Chain Integration Example

**Files:**
- Create: `examples/simple_chain/CMakeLists.txt`
- Create: `examples/simple_chain/main.cpp`

**Step 1: Create example**

```cpp
// examples/simple_chain/main.cpp
#include <boostchain/boostchain.hpp>
#include <boostchain/llm/openai_provider.hpp>
#include <iostream>

using namespace boostchain;

int main() {
    const char* api_key = std::getenv("OPENAI_API_KEY");
    if (!api_key) {
        std::cerr << "Please set OPENAI_API_KEY environment variable\n";
        return 1;
    }

    // Create LLM provider
    auto llm = std::make_shared<OpenAIProvider>(api_key);
    llm->set_model("gpt-3.5-turbo");

    // Build a chain
    Chain chain;
    chain.add_prompt("You are a helpful assistant. Answer concisely.")
         .add_llm(llm);

    // Run the chain
    auto result = chain.run({{"user_input", "What is BoostChain?"}});

    std::cout << "Response: " << result.final_output << "\n";

    return 0;
}
```

**Step 2: Create examples/CMakeLists.txt**

```cmake
add_subdirectory(simple_chain)
```

**Step 3: Create examples/simple_chain/CMakeLists.txt**

```cmake
add_executable(simple_chain main.cpp)
target_link_libraries(simple_chain boostchain::boostchain)
```

**Step 4: Update root CMakeLists.txt**

Already done in Task 1

**Step 5: Build and run example**

```bash
cd build
make simple_chain
export OPENAI_API_KEY="your-key"
./examples/simple_chain/simple_chain
```

**Step 6: Commit**

```bash
git add examples/
git commit -m "feat: add simple chain example"
```

---

## Phase 4: Agent Loop (Simplified for MVP)

### Task 11: Basic Agent Implementation

**Files:**
- Create: `include/boostchain/agent.hpp`
- Create: `src/agent.cpp`
- Test: `tests/unit/test_agent.cpp`

**Step 1: Write basic agent**

```cpp
// include/boostchain/agent.hpp
#pragma once

#include <boostchain/llm_provider.hpp>
#include <string>
#include <vector>
#include <memory>

namespace boostchain {

// Simple agent result
struct AgentResult {
    std::string final_answer;
    int steps_taken;
    bool completed;
};

// Basic Agent (simplified, full tool loop in later tasks)
class Agent {
public:
    Agent(std::shared_ptr<ILLMProvider> llm);

    // Run single-turn conversation
    AgentResult run(const std::string& user_input);

    // Set system prompt
    void set_system_prompt(const std::string& prompt);

private:
    std::shared_ptr<ILLMProvider> llm_;
    std::string system_prompt_;
    std::vector<Message> conversation_history_;
};

} // namespace boostchain
```

**Step 2: Write agent.cpp**

```cpp
// src/agent.cpp
#include <boostchain/agent.hpp>

namespace boostchain {

Agent::Agent(std::shared_ptr<ILLMProvider> llm)
    : llm_(std::move(llm)),
      system_prompt_("You are a helpful assistant.") {}

void Agent::set_system_prompt(const std::string& prompt) {
    system_prompt_ = prompt;
}

AgentResult Agent::run(const std::string& user_input) {
    // Build conversation
    std::vector<Message> messages;
    messages.push_back({Message::system, system_prompt_});

    // Add history
    for (const auto& msg : conversation_history_) {
        messages.push_back(msg);
    }

    // Add current input
    messages.push_back({Message::user, user_input});

    // Call LLM
    ChatRequest req;
    req.messages = messages;
    auto response = llm_->chat(req);

    // Update history
    conversation_history_.push_back({Message::user, user_input});
    conversation_history_.push_back(response.message);

    AgentResult result;
    result.final_answer = response.message.content;
    result.steps_taken = 1;
    result.completed = true;

    return result;
}

} // namespace boostchain
```

**Step 3: Write test**

```cpp
// tests/unit/test_agent.cpp
#include <boostchain/agent.hpp>
#include <cassert>

using namespace boostchain;

int main() {
    // Mock provider test
    // Full integration test with real LLM in tests/integration/

    Agent agent(nullptr);  // Constructor test
    agent.set_system_prompt("Test prompt");

    return 0;
}
```

**Step 4: Update CMakeLists.txt**

```cmake
target_sources(boostchain
    PRIVATE
        src/agent.cpp
)
```

**Step 5: Update tests/unit/CMakeLists.txt**

```cmake
add_executable(test_agent test_agent.cpp)
target_link_libraries(test_agent boostchain::boostchain)
add_test(NAME test_agent COMMAND test_agent)
```

**Step 6: Commit**

```bash
git add include/boostchain/agent.hpp src/agent.cpp tests/unit/test_agent.cpp
git commit -m "feat: implement basic Agent class"
```

---

## Remaining Tasks (Brief Outline)

Due to length, remaining tasks follow the same pattern:

### Phase 5: Memory System
- Task 12: IMemory interface and ShortTermMemory
- Task 13: LongTermMemory with embeddings

### Phase 6: Tool System
- Task 14: ITool interface
- Task 15: Built-in tools (HttpTool, CalculatorTool)
- Task 16: Agent-Tool integration

### Phase 7: Persistence
- Task 17: Serialization utilities
- Task 18: Agent state save/load

### Phase 8: Concurrency
- Task 19: Executor (thread pool)
- Task 20: Async and streaming APIs

### Phase 9: Additional Providers
- Task 21: Claude provider
- Task 22: Gemini provider

### Phase 10: Testing & Documentation
- Task 23: Integration tests
- Task 24: Examples
- Task 25: API documentation

---

## Completion Criteria

- [ ] All unit tests pass
- [ ] Integration tests with real LLM providers pass
- [ ] Examples compile and run
- [ ] API documentation complete
- [ ] README with quick start guide

---

## Notes for Implementation

1. **TDD Approach**: Write test first, watch it fail, implement, watch it pass
2. **Frequent Commits**: Each task ends with a commit
3. **YAGNI**: Implement only what's needed for MVP
4. **DRY**: Reuse OpenAI provider for compatible APIs

---

> **END OF IMPLEMENTATION PLAN**
