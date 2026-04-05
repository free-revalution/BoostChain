#include "query/context.hpp"
#include "git/git.hpp"
#include "utils/process.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <sstream>
#include <fstream>

namespace ccmake {

static std::string iso_date() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&time_t, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
    return buf;
}

QueryGitStatus get_git_status(const std::string& cwd) {
    QueryGitStatus qs;

    auto repo_check = run_command("git rev-parse --is-inside-work-tree", cwd);
    if (repo_check.exit_code != 0) {
        qs.is_git = false;
        return qs;
    }

    auto gs = git_status(std::filesystem::path(cwd));
    qs.is_git = gs.is_repo;
    qs.branch = gs.branch;

    // Detect main branch
    auto main_check = run_command("git symbolic-ref refs/remotes/origin/HEAD 2>/dev/null | sed 's@^refs/remotes/origin/@@'", cwd);
    if (main_check.exit_code == 0) {
        qs.main_branch = main_check.stdout_output;
        // trim whitespace
        while (!qs.main_branch.empty() && (qs.main_branch.back() == '\n' || qs.main_branch.back() == '\r' || qs.main_branch.back() == ' '))
            qs.main_branch.pop_back();
    }

    // Build status string (truncated to 2000 chars)
    std::ostringstream status_ss;
    for (const auto& f : gs.staged) status_ss << "M  " << f << "\n";
    for (const auto& f : gs.modified) status_ss << " M " << f << "\n";
    for (const auto& f : gs.untracked) status_ss << "?? " << f << "\n";
    qs.status = status_ss.str();
    if (qs.status.size() > 2000) {
        qs.status = qs.status.substr(0, 2000) + "\n... (truncated)";
    }

    // Get git user
    auto user_result = run_command("git config user.name", cwd);
    if (user_result.exit_code == 0) {
        qs.user = user_result.stdout_output;
        while (!qs.user.empty() && (qs.user.back() == '\n' || qs.user.back() == '\r'))
            qs.user.pop_back();
    }

    return qs;
}

std::string format_git_status(const QueryGitStatus& status) {
    if (!status.is_git) return "";

    std::ostringstream ss;
    ss << "Git branch: " << status.branch << "\n";
    if (!status.main_branch.empty()) {
        ss << "Main branch: " << status.main_branch << "\n";
    }
    if (!status.user.empty()) {
        ss << "Git user: " << status.user << "\n";
    }
    ss << "Git status: " << (status.status.empty() ? "clean" : "\n" + status.status);
    return ss.str();
}

std::string build_user_context(const std::string& cwd) {
    std::ostringstream ss;

    // Current date
    ss << "Current date: " << iso_date() << "\n\n";

    // Git context
    auto git = get_git_status(cwd);
    if (git.is_git) {
        ss << "Current directory: " << cwd << "\n";
        ss << "Branch: " << git.branch << "\n";
        if (git.status.empty()) {
            ss << "Status: clean\n";
        } else {
            ss << "Status:\n" << git.status;
        }
        ss << "\n";
    } else {
        ss << "Current directory: " << cwd << "\n";
        ss << "(not a git repository)\n\n";
    }

    return ss.str();
}

std::string build_system_context(const std::string& cwd) {
    auto git = get_git_status(cwd);
    if (git.is_git) {
        return format_git_status(git);
    }
    return "";
}

std::string build_system_prompt(const QueryConfig& config,
                                const std::string& user_context,
                                const std::string& system_context) {
    std::ostringstream ss;

    // Custom or default system prompt
    if (!config.system_prompt.empty()) {
        ss << config.system_prompt << "\n";
    } else {
        ss << "You are Claude Code, an interactive CLI tool for software engineering.\n";
    }

    // User context
    if (!user_context.empty()) {
        ss << "\n" << user_context;
    }

    // System context (git info)
    if (!system_context.empty()) {
        ss << "\n" << system_context;
    }

    return ss.str();
}

}  // namespace ccmake
