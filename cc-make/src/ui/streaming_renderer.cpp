#include "ui/streaming_renderer.hpp"

namespace ccmake {

StreamingRenderer::StreamingRenderer() = default;

QueryEventCallback StreamingRenderer::make_callback() {
    return [this](const QueryEvent& event) {
        handle_event(event);
    };
}

void StreamingRenderer::handle_event(const QueryEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::visit([this](const auto& e) {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, QueryEventAssistant>) {
            handle_text(e);
        } else if constexpr (std::is_same_v<T, QueryEventToolUse>) {
            handle_tool_use(e);
        } else if constexpr (std::is_same_v<T, QueryEventToolResult>) {
            handle_tool_result(e);
        } else if constexpr (std::is_same_v<T, QueryEventUsage>) {
            handle_usage(e);
        } else if constexpr (std::is_same_v<T, QueryEventError>) {
            handle_error(e);
        } else if constexpr (std::is_same_v<T, QueryEventThinking>) {
            handle_thinking(e);
        }
    }, event);
}

void StreamingRenderer::handle_text(const QueryEventAssistant& evt) {
    // Close any open tool display
    if (in_tool_) {
        std::cout << "\n";
        in_tool_ = false;
    }

    if (first_text_) {
        first_text_ = false;
    }

    in_text_ = true;
    text_output_ += evt.message.id;

    // Display assistant text blocks
    for (const auto& block : evt.message.content) {
        if (auto* text = std::get_if<TextBlock>(&block)) {
            std::cout << text->text << std::flush;
        }
    }
}

void StreamingRenderer::handle_tool_use(const QueryEventToolUse& evt) {
    if (in_text_) {
        std::cout << "\n";
        in_text_ = false;
    }

    ++tool_call_count_;
    in_tool_ = true;
    current_tool_name_ = evt.tool_name;

    std::cout << "\033[2m⚙ " << evt.tool_name;
    if (!evt.input.empty()) {
        // Show a brief summary of the tool input
        std::string summary;
        if (evt.input.contains("file_path")) {
            summary = evt.input["file_path"].get<std::string>();
        } else if (evt.input.contains("pattern")) {
            summary = evt.input["pattern"].get<std::string>();
        } else if (evt.input.contains("command")) {
            summary = evt.input["command"].get<std::string>();
        }
        if (summary.size() > 60) {
            summary = summary.substr(0, 57) + "...";
        }
        if (!summary.empty()) {
            std::cout << "(" << summary << ")";
        }
    }
    std::cout << "\033[0m" << std::flush;
}

void StreamingRenderer::handle_tool_result(const QueryEventToolResult& evt) {
    in_tool_ = false;

    if (evt.is_error) {
        ++error_count_;
        std::cout << "\033[31m✗\033[0m";
    } else {
        std::cout << "\033[32m✓\033[0m";
    }
    std::cout << std::flush;
}

void StreamingRenderer::handle_usage(const QueryEventUsage& evt) {
    // Could display token count in real-time
    (void)evt;
}

void StreamingRenderer::handle_error(const QueryEventError& evt) {
    std::cerr << "\033[31mError: " << evt.error_message << "\033[0m" << std::endl;
    ++error_count_;
}

void StreamingRenderer::handle_thinking(const QueryEventThinking& evt) {
    if (show_thinking_) {
        std::cout << "\033[2m💭 " << evt.thinking << "\033[0m" << std::flush;
    }
}

void StreamingRenderer::on_complete() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (in_text_ || in_tool_) {
        std::cout << "\n";
    }

    if (tool_call_count_ > 0) {
        std::cout << "\033[2m["
                  << tool_call_count_ << " tool call" << (tool_call_count_ > 1 ? "s" : "");
        if (error_count_ > 0) {
            std::cout << ", " << error_count_ << " error" << (error_count_ > 1 ? "s" : "");
        }
        std::cout << "]\033[0m" << std::endl;
    }
}

std::string StreamingRenderer::text_output() const {
    return text_output_;
}

int StreamingRenderer::tool_call_count() const {
    return tool_call_count_;
}

int StreamingRenderer::error_count() const {
    return error_count_;
}

void StreamingRenderer::reset() {
    text_output_.clear();
    tool_call_count_ = 0;
    error_count_ = 0;
    in_text_ = false;
    first_text_ = true;
    in_tool_ = false;
    current_tool_name_.clear();
}

void StreamingRenderer::set_show_thinking(bool show) {
    show_thinking_ = show;
}

void StreamingRenderer::flush() {
    std::cout << std::flush;
}

}  // namespace ccmake
