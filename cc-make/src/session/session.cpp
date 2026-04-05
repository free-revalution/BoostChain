#include "session/session.hpp"
#include "core/json_utils.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <chrono>
#include <iomanip>
#include <filesystem>

namespace ccmake {

SessionStore::SessionStore(const std::filesystem::path& base_dir)
    : base_dir_(base_dir) {}

std::string SessionStore::generate_session_id() {
    static const char chars[] = "abcdef0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 15);

    std::string id;
    for (int i = 0; i < 8; ++i) {
        id += chars[dist(gen)];
    }
    return id;
}

std::filesystem::path SessionStore::session_dir(const std::filesystem::path& base, const std::string& id) {
    return base / id;
}

std::string SessionStore::current_time_iso8601() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::string SessionStore::create_session(const std::string& model, const std::string& cwd) {
    auto id = generate_session_id();
    auto dir = session_dir(base_dir_, id);
    std::filesystem::create_directories(dir);

    SessionMeta meta;
    meta.id = id;
    meta.model = model;
    meta.created_at = current_time_iso8601();
    meta.message_count = 0;
    meta.cwd = cwd;

    nlohmann::json j;
    j["id"] = meta.id;
    j["model"] = meta.model;
    j["created_at"] = meta.created_at;
    j["message_count"] = 0;
    j["cwd"] = meta.cwd;

    std::ofstream ofs(dir / "meta.json");
    ofs << j.dump(2);

    // Create empty messages file
    nlohmann::json msgs;
    msgs["messages"] = nlohmann::json::array();
    std::ofstream mfs(dir / "messages.json");
    mfs << msgs.dump(2);

    return id;
}

bool SessionStore::save_session(const std::string& session_id,
                                const std::vector<Message>& messages,
                                const std::string& model,
                                const std::string& cwd) {
    auto dir = session_dir(base_dir_, session_id);
    if (!std::filesystem::is_directory(dir)) return false;

    // Update meta
    SessionMeta meta;
    meta.id = session_id;
    meta.model = model;
    meta.cwd = cwd;
    meta.message_count = static_cast<int>(messages.size());

    auto existing_meta = load_meta(session_id);
    if (existing_meta) {
        meta.created_at = existing_meta->created_at;
    } else {
        meta.created_at = current_time_iso8601();
    }

    nlohmann::json meta_j;
    meta_j["id"] = meta.id;
    meta_j["model"] = meta.model;
    meta_j["created_at"] = meta.created_at;
    meta_j["message_count"] = meta.message_count;
    meta_j["cwd"] = meta.cwd;

    std::ofstream ofs(dir / "meta.json");
    ofs << meta_j.dump(2);

    // Save messages
    nlohmann::json msg_j;
    msg_j["messages"] = nlohmann::json::array();

    for (const auto& msg : messages) {
        nlohmann::json m;
        m["id"] = msg.id;
        m["role"] = (msg.role == MessageRole::User) ? "user" : "assistant";

        m["content"] = nlohmann::json::array();
        for (const auto& block : msg.content) {
            std::visit([&m](const auto& b) {
                using T = std::decay_t<decltype(b)>;
                if constexpr (std::is_same_v<T, TextBlock>) {
                    nlohmann::json cb;
                    cb["type"] = "text";
                    cb["text"] = b.text;
                    m["content"].push_back(cb);
                } else if constexpr (std::is_same_v<T, ToolUseBlock>) {
                    nlohmann::json cb;
                    cb["type"] = "tool_use";
                    cb["id"] = b.id;
                    cb["name"] = b.name;
                    cb["input"] = b.input;
                    m["content"].push_back(cb);
                } else if constexpr (std::is_same_v<T, ToolResultBlock>) {
                    nlohmann::json cb;
                    cb["type"] = "tool_result";
                    cb["tool_use_id"] = b.tool_use_id;
                    cb["content"] = b.content;
                    cb["is_error"] = b.is_error;
                    m["content"].push_back(cb);
                } else if constexpr (std::is_same_v<T, ThinkingBlock>) {
                    nlohmann::json cb;
                    cb["type"] = "thinking";
                    cb["thinking"] = b.thinking;
                    cb["signature"] = b.signature;
                    m["content"].push_back(cb);
                }
            }, block);
        }

        if (msg.stop_reason.has_value()) {
            std::string sr;
            switch (*msg.stop_reason) {
                case StopReason::EndTurn: sr = "end_turn"; break;
                case StopReason::MaxTokens: sr = "max_tokens"; break;
                case StopReason::ToolUse: sr = "tool_use"; break;
                case StopReason::StopSequence: sr = "stop_sequence"; break;
            }
            m["stop_reason"] = sr;
        }

        m["usage"]["total_input_tokens"] = msg.usage.total_input_tokens;
        m["usage"]["total_output_tokens"] = msg.usage.total_output_tokens;

        msg_j["messages"].push_back(m);
    }

    std::ofstream mfs(dir / "messages.json");
    mfs << msg_j.dump(2);

    return true;
}

std::optional<std::vector<Message>> SessionStore::load_session(const std::string& session_id) {
    auto dir = session_dir(base_dir_, session_id);
    auto path = dir / "messages.json";

    if (!std::filesystem::exists(path)) return std::nullopt;

    std::ifstream ifs(path);
    if (!ifs.is_open()) return std::nullopt;

    std::ostringstream oss;
    oss << ifs.rdbuf();
    auto json_result = json_safe_parse(oss.str());
    if (!json_result.has_value()) return std::nullopt;

    const auto& j = json_result.value();
    if (!j.contains("messages") || !j["messages"].is_array()) return std::nullopt;

    std::vector<Message> messages;
    for (const auto& msg_j : j["messages"]) {
        Message msg;

        msg.id = json_get_or<std::string>(msg_j, "id", "");
        std::string role_str = json_get_or<std::string>(msg_j, "role", "user");
        msg.role = (role_str == "assistant") ? MessageRole::Assistant : MessageRole::User;

        if (msg_j.contains("content") && msg_j["content"].is_array()) {
            for (const auto& cb : msg_j["content"]) {
                std::string type = json_get_or<std::string>(cb, "type", "");
                if (type == "text") {
                    msg.content.emplace_back(TextBlock{json_get_or<std::string>(cb, "text", "")});
                } else if (type == "tool_use") {
                    ToolUseBlock tb;
                    tb.id = json_get_or<std::string>(cb, "id", "");
                    tb.name = json_get_or<std::string>(cb, "name", "");
                    if (cb.contains("input")) tb.input = cb["input"];
                    msg.content.emplace_back(std::move(tb));
                } else if (type == "tool_result") {
                    ToolResultBlock tr;
                    tr.tool_use_id = json_get_or<std::string>(cb, "tool_use_id", "");
                    tr.content = json_get_or<std::string>(cb, "content", "");
                    tr.is_error = json_get_or<bool>(cb, "is_error", false);
                    msg.content.emplace_back(std::move(tr));
                } else if (type == "thinking") {
                    ThinkingBlock th;
                    th.thinking = json_get_or<std::string>(cb, "thinking", "");
                    th.signature = json_get_or<std::string>(cb, "signature", "");
                    msg.content.emplace_back(std::move(th));
                }
            }
        }

        std::string sr = json_get_or<std::string>(msg_j, "stop_reason", "");
        if (sr == "end_turn") msg.stop_reason = StopReason::EndTurn;
        else if (sr == "max_tokens") msg.stop_reason = StopReason::MaxTokens;
        else if (sr == "tool_use") msg.stop_reason = StopReason::ToolUse;
        else if (sr == "stop_sequence") msg.stop_reason = StopReason::StopSequence;

        if (msg_j.contains("usage")) {
            msg.usage.total_input_tokens = json_get_or<int64_t>(msg_j["usage"], "total_input_tokens", 0);
            msg.usage.total_output_tokens = json_get_or<int64_t>(msg_j["usage"], "total_output_tokens", 0);
        }

        messages.push_back(std::move(msg));
    }

    return messages;
}

std::optional<SessionMeta> SessionStore::load_meta(const std::string& session_id) {
    auto dir = session_dir(base_dir_, session_id);
    auto path = dir / "meta.json";

    if (!std::filesystem::exists(path)) return std::nullopt;

    std::ifstream ifs(path);
    if (!ifs.is_open()) return std::nullopt;

    std::ostringstream oss;
    oss << ifs.rdbuf();
    auto json_result = json_safe_parse(oss.str());
    if (!json_result.has_value()) return std::nullopt;

    const auto& j = json_result.value();
    SessionMeta meta;
    meta.id = json_get_or<std::string>(j, "id", session_id);
    meta.model = json_get_or<std::string>(j, "model", "");
    meta.created_at = json_get_or<std::string>(j, "created_at", "");
    meta.message_count = json_get_or<int>(j, "message_count", 0);
    meta.cwd = json_get_or<std::string>(j, "cwd", "");

    return meta;
}

std::vector<SessionMeta> SessionStore::list_sessions() {
    std::vector<SessionMeta> sessions;

    if (!std::filesystem::is_directory(base_dir_)) return sessions;

    for (const auto& entry : std::filesystem::directory_iterator(base_dir_)) {
        if (!entry.is_directory()) continue;

        auto meta = load_meta(entry.path().filename().string());
        if (meta) {
            sessions.push_back(std::move(*meta));
        }
    }

    // Sort by created_at descending (newest first)
    std::sort(sessions.begin(), sessions.end(),
        [](const SessionMeta& a, const SessionMeta& b) {
            return a.created_at > b.created_at;
        });

    return sessions;
}

bool SessionStore::delete_session(const std::string& session_id) {
    auto dir = session_dir(base_dir_, session_id);
    if (!std::filesystem::is_directory(dir)) return false;
    std::filesystem::remove_all(dir);
    return true;
}

std::filesystem::path SessionStore::base_dir() const {
    return base_dir_;
}

}  // namespace ccmake
