#pragma once

#include "api/types.hpp"
#include <string>
#include <optional>

namespace ccmake {

APIError parse_api_error(int status_code, const std::string& body);
APIError make_connection_error(const std::string& message);
bool should_retry_error(const APIError& error);
int get_retry_delay_ms(int attempt, std::optional<std::string> retry_after);
std::string user_facing_error_message(const APIError& error);

}  // namespace ccmake
