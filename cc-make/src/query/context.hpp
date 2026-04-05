#pragma once

#include "query/types.hpp"
#include <string>

namespace ccmake {

// Query-scoped git status (distinct from git module's GitStatus)
struct QueryGitStatus {
    bool is_git = false;
    std::string branch;
    std::string main_branch;
    std::string status;
    std::string user;
};

QueryGitStatus get_git_status(const std::string& cwd);
std::string format_git_status(const QueryGitStatus& status);
std::string build_user_context(const std::string& cwd);
std::string build_system_context(const std::string& cwd);
std::string build_system_prompt(const QueryConfig& config,
                                const std::string& user_context,
                                const std::string& system_context);

}  // namespace ccmake
