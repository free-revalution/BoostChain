#pragma once
#include <string>
#include <vector>
#include <optional>

namespace ccmake {

struct SSEEvent {
    std::string event_type;
    std::string data;
    std::string id;
    std::optional<int> retry_ms;
};

class SSEParser {
public:
    std::vector<SSEEvent> feed(const std::string& data);
    void reset();

private:
    std::string buffer_;
    SSEEvent current_;
    bool data_field_seen_ = false;

    void process_line(const std::string& line, std::vector<SSEEvent>& events);
    void emit_current(std::vector<SSEEvent>& events);
};

}  // namespace ccmake
