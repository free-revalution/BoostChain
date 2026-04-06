#pragma once

#include "core/result.hpp"

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

namespace ccmake {

struct GitStatus {
    std::string branch;
    std::vector<std::string> modified;
    std::vector<std::string> staged;
    std::vector<std::string> untracked;
    bool is_repo = false;
};

struct GitDiffFile {
    std::string path;
    std::string diff;
    bool staged;
};

GitStatus git_status(const std::filesystem::path& dir);
std::string git_current_branch(const std::filesystem::path& dir);
std::vector<GitDiffFile> git_diff(const std::filesystem::path& dir);
bool git_is_dirty(const std::filesystem::path& dir);

struct GitLogEntry {
    std::string hash;
    std::string message;
    std::string author;
    std::string date;
};
std::vector<GitLogEntry> git_log(const std::filesystem::path& dir, int count = 5);

struct GitWorktree {
    std::string path;
    std::string branch;
    bool is_main = false;
};

Result<std::vector<GitWorktree>, std::string> git_worktree_list(const std::filesystem::path& cwd = ".");
Result<bool, std::string> git_worktree_add(const std::filesystem::path& cwd, const std::string& path, const std::string& branch);
Result<bool, std::string> git_worktree_remove(const std::filesystem::path& cwd, const std::string& path);

}  // namespace ccmake
