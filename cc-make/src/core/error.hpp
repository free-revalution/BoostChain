#pragma once

#include <stdexcept>
#include <string>
#include <chrono>

namespace ccmake {

class CcError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ApiError : public CcError {
public:
    int status_code;
    std::string error_type;
    ApiError(int code, std::string type, std::string message);
};

class AuthError : public CcError {
public:
    using CcError::CcError;
};

class ConfigError : public CcError {
public:
    using CcError::CcError;
};

class ToolError : public CcError {
public:
    using CcError::CcError;
};

class McpError : public CcError {
public:
    int error_code;
    McpError(int code, std::string message);
};

class PermissionDeniedError : public CcError {
public:
    using CcError::CcError;
};

class RateLimitError : public ApiError {
public:
    std::chrono::seconds retry_after;
    RateLimitError(int code, std::string type, std::string message, std::chrono::seconds ra);
};

class OverloadedError : public ApiError {
public:
    using ApiError::ApiError;
};

class PromptTooLongError : public CcError {
public:
    using CcError::CcError;
};

}  // namespace ccmake
