#include "api/errors.hpp"

#include <nlohmann/json.hpp>
#include <random>
#include <cmath>

namespace ccmake {

static bool is_retryable_status(int status_code) {
    return status_code == 408
        || status_code == 409
        || status_code == 429
        || (status_code >= 500 && status_code <= 599)
        || status_code == 529;
}

APIError parse_api_error(int status_code, const std::string& body) {
    APIError error;
    error.status_code = status_code;

    // Try to parse JSON body
    try {
        auto doc = nlohmann::json::parse(body);

        // Extract error.type and error.message from nested error object
        if (doc.contains("error") && doc["error"].is_object()) {
            const auto& err_obj = doc["error"];
            if (err_obj.contains("type") && err_obj["type"].is_string()) {
                error.error_type = err_obj["type"].get<std::string>();
            }
            if (err_obj.contains("message") && err_obj["message"].is_string()) {
                error.message = err_obj["message"].get<std::string>();
            }
        } else {
            // Fallback: top-level type/message
            if (doc.contains("type") && doc["type"].is_string()) {
                error.error_type = doc["type"].get<std::string>();
            }
            if (doc.contains("message") && doc["message"].is_string()) {
                error.message = doc["message"].get<std::string>();
            }
        }

        // Check for overloaded in body
        std::string body_lower = body;
        std::transform(body_lower.begin(), body_lower.end(), body_lower.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (body_lower.find("overloaded_error") != std::string::npos) {
            error.is_overloaded = true;
        }
    } catch (...) {
        // Malformed JSON: rely on status code defaults
    }

    // Map status code to retryability
    if (is_retryable_status(status_code)) {
        error.should_retry = true;
    } else if (status_code >= 400 && status_code < 500) {
        error.should_retry = false;
    }

    // Specific status code mappings
    if (status_code == 429) {
        error.error_type = "rate_limit_error";
        error.should_retry = true;
    }
    if (status_code == 529) {
        error.is_overloaded = true;
        error.should_retry = true;
    }

    return error;
}

APIError make_connection_error(const std::string& message) {
    APIError error;
    error.status_code = 0;
    error.error_type = "connection_error";
    error.message = message;
    error.should_retry = true;
    return error;
}

bool should_retry_error(const APIError& error) {
    // Connection errors always retryable
    if (error.status_code == 0 && error.should_retry) {
        return true;
    }
    if (error.is_overloaded) {
        return true;
    }
    return is_retryable_status(error.status_code);
}

int get_retry_delay_ms(int attempt, std::optional<std::string> retry_after) {
    // If retry_after header is provided, use it
    if (retry_after.has_value()) {
        try {
            double seconds = std::stod(retry_after.value());
            return static_cast<int>(seconds * 1000.0);
        } catch (...) {
            // Fall through to exponential backoff if parsing fails
        }
    }

    // Exponential backoff: min(500 * 2^(attempt-1), 32000) + jitter
    int base = 500;
    int delay = base;
    for (int i = 1; i < attempt; ++i) {
        delay *= 2;
        if (delay > 32000) {
            delay = 32000;
            break;
        }
    }

    // Add jitter: random(0, 0.25 * base)
    std::random_device rd;
    std::mt19937 gen(rd());
    int jitter_max = delay / 4;
    std::uniform_int_distribution<> dist(0, jitter_max);
    int jitter = dist(gen);

    return delay + jitter;
}

std::string user_facing_error_message(const APIError& error) {
    if (error.error_type == "authentication_error") {
        return "Authentication failed: " + error.message;
    }
    if (error.error_type == "rate_limit_error") {
        return "Rate limited: " + error.message;
    }
    if (error.error_type == "connection_error") {
        return "Connection error: " + error.message;
    }
    if (error.is_overloaded) {
        return "API overloaded, retrying: " + error.message;
    }
    if (error.error_type == "invalid_request_error") {
        return "Invalid request: " + error.message;
    }
    return "API error (" + std::to_string(error.status_code) + "): " + error.message;
}

}  // namespace ccmake
