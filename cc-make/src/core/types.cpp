#include "core/types.hpp"

#include <nlohmann/json.hpp>

namespace ccmake {

Message Message::user(std::string id, std::string text) {
    Message msg;
    msg.id = std::move(id);
    msg.role = MessageRole::User;
    msg.content.emplace_back(TextBlock{std::move(text)});
    return msg;
}

Message Message::assistant(std::string id, std::vector<ContentBlock> content,
                            std::optional<StopReason> stop) {
    Message msg;
    msg.id = std::move(id);
    msg.role = MessageRole::Assistant;
    msg.content = std::move(content);
    msg.stop_reason = std::move(stop);
    return msg;
}

}  // namespace ccmake
