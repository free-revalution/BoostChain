#include "api/sse_parser.hpp"
#include <sstream>

namespace ccmake {

std::vector<SSEEvent> SSEParser::feed(const std::string& data) {
    buffer_.append(data);

    // Strip UTF-8 BOM if present at the start of buffer
    if (buffer_.size() >= 3 &&
        static_cast<unsigned char>(buffer_[0]) == 0xEF &&
        static_cast<unsigned char>(buffer_[1]) == 0xBB &&
        static_cast<unsigned char>(buffer_[2]) == 0xBF) {
        buffer_.erase(0, 3);
    }

    std::vector<SSEEvent> events;

    size_t pos = 0;
    size_t len = buffer_.size();

    while (pos < len) {
        size_t nl = buffer_.find('\n', pos);
        if (nl == std::string::npos) {
            // Incomplete line — keep remainder in buffer
            break;
        }

        // Extract the line, stripping \r before \n
        std::string line = buffer_.substr(pos, nl - pos);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        pos = nl + 1;
        process_line(line, events);
    }

    // Keep incomplete trailing data in buffer
    buffer_ = buffer_.substr(pos);

    return events;
}

void SSEParser::process_line(const std::string& line, std::vector<SSEEvent>& events) {
    if (line.empty()) {
        // Empty line triggers event emission
        if (!current_.data.empty() || !current_.event_type.empty()) {
            emit_current(events);
        }
        current_ = SSEEvent{};
        return;
    }

    // Comment line — ignore
    if (line[0] == ':') {
        return;
    }

    // Find the first colon
    size_t colon = line.find(':');
    std::string field;
    std::string value;

    if (colon == std::string::npos) {
        // No colon — field name only, empty value
        field = line;
    } else {
        field = line.substr(0, colon);
        value = line.substr(colon + 1);
        // Strip leading single space from value
        if (!value.empty() && value[0] == ' ') {
            value.erase(0, 1);
        }
    }

    if (field == "event") {
        current_.event_type = value;
    } else if (field == "data") {
        if (current_.data.empty()) {
            current_.data = value;
        } else {
            current_.data += "\n" + value;
        }
    } else if (field == "id") {
        current_.id = value;
    } else if (field == "retry") {
        try {
            current_.retry_ms = std::stoi(value);
        } catch (...) {
            // Invalid retry value — ignore
        }
    }
    // Unknown fields are ignored per SSE spec
}

void SSEParser::emit_current(std::vector<SSEEvent>& events) {
    events.push_back(current_);
}

void SSEParser::reset() {
    buffer_.clear();
    current_ = SSEEvent{};
}

}  // namespace ccmake
