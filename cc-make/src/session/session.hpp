#pragma once

#include "core/types.hpp"

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

namespace ccmake {

struct SessionMeta {
    std::string id;
    std::string model;
    std::string created_at;  // ISO 8601
    int message_count = 0;
    std::string cwd;
};

class SessionStore {
public:
    explicit SessionStore(const std::filesystem::path& base_dir = "");

    // Create a new session and return its ID
    std::string create_session(const std::string& model, const std::string& cwd);

    // Save messages to an existing session
    bool save_session(const std::string& session_id,
                      const std::vector<Message>& messages,
                      const std::string& model,
                      const std::string& cwd);

    // Load messages from a session
    std::optional<std::vector<Message>> load_session(const std::string& session_id);

    // Load session metadata
    std::optional<SessionMeta> load_meta(const std::string& session_id);

    // List all sessions (sorted by creation time, newest first)
    std::vector<SessionMeta> list_sessions();

    // Delete a session
    bool delete_session(const std::string& session_id);

    // Get the base directory
    std::filesystem::path base_dir() const;

private:
    static std::string generate_session_id();
    static std::filesystem::path session_dir(const std::filesystem::path& base, const std::string& id);
    static std::string current_time_iso8601();

    std::filesystem::path base_dir_;
};

}  // namespace ccmake
