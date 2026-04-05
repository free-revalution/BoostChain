#pragma once

#include "query/types.hpp"

#include <string>
#include <functional>
#include <mutex>
#include <iostream>

namespace ccmake {

// Renders streaming events to stdout in real-time
class StreamingRenderer {
public:
    // Callback type for when streaming completes
    using CompletionCallback = std::function<void()>;

    StreamingRenderer();

    // Create a QueryEventCallback that can be passed to the agentic loop
    QueryEventCallback make_callback();

    // Called when the entire query turn is complete
    void on_complete();

    // Get the accumulated text output
    std::string text_output() const;

    // Get tool call count
    int tool_call_count() const;

    // Get error count
    int error_count() const;

    // Reset state for a new turn
    void reset();

    // Enable/disable thinking display
    void set_show_thinking(bool show);

    // Flush any pending output
    void flush();

private:
    void handle_event(const QueryEvent& event);
    void handle_text(const QueryEventAssistant& evt);
    void handle_tool_use(const QueryEventToolUse& evt);
    void handle_tool_result(const QueryEventToolResult& evt);
    void handle_usage(const QueryEventUsage& evt);
    void handle_error(const QueryEventError& evt);
    void handle_thinking(const QueryEventThinking& evt);

    std::mutex mutex_;
    std::string text_output_;
    int tool_call_count_ = 0;
    int error_count_ = 0;
    bool show_thinking_ = false;
    bool in_text_ = false;
    bool first_text_ = true;
    bool in_tool_ = false;
    std::string current_tool_name_;
};

}  // namespace ccmake
