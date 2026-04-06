#include "git/git.hpp"
#include "utils/process.hpp"
#include "core/string_utils.hpp"

#include <sstream>

namespace ccmake {

GitStatus git_status(const std::filesystem::path& dir) {
    GitStatus status;

    // Check if inside a git work tree
    auto repo_check = run_command("git rev-parse --is-inside-work-tree", dir.string());
    if (repo_check.exit_code != 0 || trim(repo_check.stdout_output) != "true") {
        status.is_repo = false;
        return status;
    }
    status.is_repo = true;

    // Get current branch
    auto branch_result = run_command("git branch --show-current", dir.string());
    if (branch_result.exit_code == 0) {
        status.branch = trim(branch_result.stdout_output);
    }

    // Get porcelain status
    auto porcelain = run_command("git status --porcelain=v1", dir.string());
    if (porcelain.exit_code != 0) {
        return status;
    }

    auto lines = split(trim(porcelain.stdout_output), '\n');
    for (const auto& line : lines) {
        if (line.size() < 3) continue;

        char index_status = line[0];   // staged status
        char work_status = line[1];    // unstaged status

        // Extract file path(s)
        std::string file_path;
        if (index_status == 'R') {
            // Renamed: "R  old -> new"
            auto arrow_pos = line.find("-> ");
            if (arrow_pos != std::string::npos) {
                file_path = line.substr(arrow_pos + 3);
            } else {
                file_path = trim(line.substr(2));
            }
        } else {
            file_path = trim(line.substr(2));
        }

        if (file_path.empty()) continue;

        if (work_status == '?' || index_status == '?') {
            status.untracked.push_back(file_path);
        } else {
            if (index_status != ' ' && index_status != '?') {
                status.staged.push_back(file_path);
            }
            if (work_status != ' ' && work_status != '?') {
                // Only add to modified if not already in staged, or it's both
                status.modified.push_back(file_path);
            }
            // If file is only in index (no work tree changes), it's staged but not modified
            if (index_status != ' ' && index_status != '?' && work_status == ' ') {
                // Already added to staged above; don't add duplicate to modified
            }
        }
    }

    return status;
}

std::string git_current_branch(const std::filesystem::path& dir) {
    auto result = run_command("git branch --show-current", dir.string());
    if (result.exit_code != 0) {
        return "";
    }
    return trim(result.stdout_output);
}

std::vector<GitDiffFile> git_diff(const std::filesystem::path& dir) {
    std::vector<GitDiffFile> diffs;

    // Get list of modified files (unstaged)
    auto files_result = run_command("git diff --name-only", dir.string());
    if (files_result.exit_code == 0 && !trim(files_result.stdout_output).empty()) {
        auto files = split(trim(files_result.stdout_output), '\n');
        for (const auto& file : files) {
            std::string trimmed_file = trim(file);
            if (trimmed_file.empty()) continue;

            auto diff_result = run_command("git diff -- " + trimmed_file, dir.string());
            GitDiffFile df;
            df.path = trimmed_file;
            df.diff = diff_result.exit_code == 0 ? diff_result.stdout_output : "";
            df.staged = false;
            diffs.push_back(std::move(df));
        }
    }

    // Get list of staged files
    auto staged_files_result = run_command("git diff --name-only --cached", dir.string());
    if (staged_files_result.exit_code == 0 && !trim(staged_files_result.stdout_output).empty()) {
        auto files = split(trim(staged_files_result.stdout_output), '\n');
        for (const auto& file : files) {
            std::string trimmed_file = trim(file);
            if (trimmed_file.empty()) continue;

            auto diff_result = run_command("git diff --cached -- " + trimmed_file, dir.string());
            GitDiffFile df;
            df.path = trimmed_file;
            df.diff = diff_result.exit_code == 0 ? diff_result.stdout_output : "";
            df.staged = true;
            diffs.push_back(std::move(df));
        }
    }

    return diffs;
}

bool git_is_dirty(const std::filesystem::path& dir) {
    auto result = run_command("git status --porcelain", dir.string());
    if (result.exit_code != 0) {
        return false;
    }
    return !trim(result.stdout_output).empty();
}

std::vector<GitLogEntry> git_log(const std::filesystem::path& dir, int count) {
    std::vector<GitLogEntry> entries;

    std::ostringstream cmd;
    cmd << "git log -n " << count << " --format=%H%x01%s%x01%an%x01%ai";
    auto result = run_command(cmd.str(), dir.string());
    if (result.exit_code != 0 || trim(result.stdout_output).empty()) {
        return entries;
    }

    auto lines = split(trim(result.stdout_output), '\n');
    for (const auto& line : lines) {
        if (line.empty()) continue;

        auto fields = split(line, static_cast<char>(0x01));
        GitLogEntry entry;
        if (fields.size() >= 1) entry.hash = trim(fields[0]);
        if (fields.size() >= 2) entry.message = trim(fields[1]);
        if (fields.size() >= 3) entry.author = trim(fields[2]);
        if (fields.size() >= 4) entry.date = trim(fields[3]);
        entries.push_back(std::move(entry));
    }

    return entries;
}

Result<std::vector<GitWorktree>, std::string> git_worktree_list(const std::filesystem::path& cwd) {
    auto result = run_command("git worktree list --porcelain", cwd.string());
    if (result.exit_code != 0) {
        return Result<std::vector<GitWorktree>, std::string>::err(
            "git worktree list failed: " + result.stderr_output);
    }

    std::vector<GitWorktree> worktrees;
    auto lines = split(trim(result.stdout_output), '\n');

    GitWorktree current;
    bool first = true;
    for (const auto& line : lines) {
        if (starts_with(line, "worktree ")) {
            if (!current.path.empty()) {
                worktrees.push_back(std::move(current));
            }
            current = GitWorktree{};
            current.path = trim(line.substr(9));
            current.is_main = first;
            first = false;
        } else if (starts_with(line, "branch ")) {
            std::string ref = trim(line.substr(7));
            if (starts_with(ref, "refs/heads/")) {
                current.branch = ref.substr(11);
            } else {
                current.branch = ref;
            }
        } else if (line == "bare") {
            // bare repos don't have worktrees in the usual sense; skip
            current.path.clear();
        }
    }
    if (!current.path.empty()) {
        worktrees.push_back(std::move(current));
    }

    return Result<std::vector<GitWorktree>, std::string>::ok(std::move(worktrees));
}

Result<bool, std::string> git_worktree_add(const std::filesystem::path& cwd, const std::string& path, const std::string& branch) {
    // Check if the branch already exists
    auto branch_check = run_command("git show-ref --verify --quiet refs/heads/" + branch, cwd.string());
    std::string cmd;
    if (branch_check.exit_code == 0) {
        // Branch exists, checkout existing branch
        cmd = "git worktree add " + path + " " + branch;
    } else {
        // Create new branch
        cmd = "git worktree add " + path + " -b " + branch;
    }

    auto result = run_command(cmd, cwd.string());
    if (result.exit_code != 0) {
        return Result<bool, std::string>::err(
            "git worktree add failed: " + result.stderr_output);
    }
    return Result<bool, std::string>::ok(true);
}

Result<bool, std::string> git_worktree_remove(const std::filesystem::path& cwd, const std::string& path) {
    auto result = run_command("git worktree remove --force " + path, cwd.string());
    if (result.exit_code != 0) {
        return Result<bool, std::string>::err(
            "git worktree remove failed: " + result.stderr_output);
    }
    return Result<bool, std::string>::ok(true);
}

}  // namespace ccmake
