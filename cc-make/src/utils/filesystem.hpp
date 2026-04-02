#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace ccmake {

/// Recursively find all files with the given extension in a directory.
std::vector<std::filesystem::path> find_files_by_extension(
    const std::filesystem::path& dir,
    const std::string& extension
);

/// Read the entire contents of a file. Returns nullopt if the file does not exist.
std::optional<std::string> read_file(const std::filesystem::path& path);

/// Read the entire contents of a file. Throws std::runtime_error on failure.
std::string read_file_or_throw(const std::filesystem::path& path);

/// Write the given content to a file, creating it if necessary (overwrites).
bool write_file(const std::filesystem::path& path, const std::string& content);

/// Append the given content to a file, creating it if necessary.
bool append_file(const std::filesystem::path& path, const std::string& content);

/// Check whether the given directory is inside a git repository.
bool is_git_repo(const std::filesystem::path& dir);

/// Return the top-level directory of the git repository, or nullopt.
std::optional<std::filesystem::path> git_top_level(const std::filesystem::path& dir);

/// Create a directory (and parents) if it does not already exist. Returns true on success.
bool ensure_directory(const std::filesystem::path& dir);

/// Return the size of a file in bytes, or nullopt if the file does not exist.
std::optional<uint64_t> file_size(const std::filesystem::path& path);

/// Heuristic check: read the first few bytes and look for binary content.
bool is_text_file(const std::filesystem::path& path);

/// Compute the relative path from base to target.
std::filesystem::path relative_path(
    const std::filesystem::path& base,
    const std::filesystem::path& target
);

}  // namespace ccmake
