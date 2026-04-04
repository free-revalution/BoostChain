#include "api/types.hpp"
#include <stdexcept>

namespace ccmake {

nlohmann::json api_request_to_json(const APIRequest& req) {
    throw std::runtime_error("api_request_to_json not implemented");
}

APIStreamEvent parse_stream_event(const std::string& event_type, const std::string& data) {
    throw std::runtime_error("parse_stream_event not implemented");
}

APIError parse_error_response(int status_code, const std::string& body) {
    throw std::runtime_error("parse_error_response not implemented");
}

}  // namespace ccmake
