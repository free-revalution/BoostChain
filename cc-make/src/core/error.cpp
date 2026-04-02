#include "core/error.hpp"

namespace ccmake {

ApiError::ApiError(int code, std::string type, std::string message)
    : CcError(std::move(message)), status_code(code), error_type(std::move(type)) {}

McpError::McpError(int code, std::string message)
    : CcError(std::move(message)), error_code(code) {}

RateLimitError::RateLimitError(int code, std::string type, std::string message, std::chrono::seconds ra)
    : ApiError(code, std::move(type), std::move(message)), retry_after(ra) {}

}  // namespace ccmake
