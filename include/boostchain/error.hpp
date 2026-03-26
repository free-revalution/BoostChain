#pragma once

#include <stdexcept>
#include <string>

namespace boostchain {

class Error : public std::runtime_error {
public:
    explicit Error(const std::string& msg) : std::runtime_error(msg) {}
    virtual ~Error() = default;
};

class NetworkError : public Error {
public:
    explicit NetworkError(const std::string& msg, int status_code = 0)
        : Error(msg), status_code_(status_code) {}

    int status_code() const { return status_code_; }

private:
    int status_code_;
};

class APIError : public Error {
public:
    explicit APIError(const std::string& msg,
                     const std::string& raw_response = "")
        : Error(msg), raw_response_(raw_response) {}

    const std::string& raw_response() const { return raw_response_; }

private:
    std::string raw_response_;
};

class ToolError : public Error {
public:
    explicit ToolError(const std::string& tool_name,
                      const std::string& reason)
        : Error(tool_name + ": " + reason), tool_name_(tool_name) {}

    const std::string& tool_name() const { return tool_name_; }

private:
    std::string tool_name_;
};

class SerializationError : public Error {
public:
    explicit SerializationError(const std::string& msg)
        : Error(msg) {}
};

class ConfigError : public Error {
public:
    explicit ConfigError(const std::string& msg)
        : Error(msg) {}
};

} // namespace boostchain
