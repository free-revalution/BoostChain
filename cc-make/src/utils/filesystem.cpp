#include "utils/filesystem.hpp"
#include "utils/process.hpp"

#include <array>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace ccmake {

std::vector<fs::path> find_files_by_extension(const fs::path& dir, const std::string& extension) {
    std::vector<fs::path> result;
    std::error_code ec;

    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) {
        return result;
    }

    std::string ext = extension;
    // Ensure the extension starts with a dot.
    if (!ext.empty() && ext[0] != '.') {
        ext = "." + ext;
    }

    for (auto it = fs::recursive_directory_iterator(dir, fs::directory_options::skip_permission_denied, ec);
         it != fs::recursive_directory_iterator();
         it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }

        const auto& entry = *it;
        if (entry.is_regular_file(ec)) {
            const auto& path = entry.path();
            if (path.extension() == ext) {
                result.push_back(path);
            }
        }
    }

    return result;
}

std::optional<std::string> read_file(const fs::path& path) {
    std::error_code ec;
    if (!fs::exists(path, ec)) {
        return std::nullopt;
    }

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        return std::nullopt;
    }

    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

std::string read_file_or_throw(const fs::path& path) {
    auto content = read_file(path);
    if (!content.has_value()) {
        throw std::runtime_error("failed to read file: " + path.string());
    }
    return std::move(content.value());
}

bool write_file(const fs::path& path, const std::string& content) {
    // Ensure parent directory exists.
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);

    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open()) {
        return false;
    }
    ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
    return ofs.good();
}

bool append_file(const fs::path& path, const std::string& content) {
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);

    std::ofstream ofs(path, std::ios::binary | std::ios::app);
    if (!ofs.is_open()) {
        return false;
    }
    ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
    return ofs.good();
}

bool is_git_repo(const fs::path& dir) {
    auto result = run_command("git rev-parse --is-inside-work-tree", dir.string());
    // git rev-parse --is-inside-work-tree prints "true" and exits 0 if inside a repo.
    return result.exit_code == 0 && result.stdout_output.find("true") != std::string::npos;
}

std::optional<fs::path> git_top_level(const fs::path& dir) {
    auto result = run_command("git rev-parse --show-toplevel", dir.string());
    if (result.exit_code == 0 && !result.stdout_output.empty()) {
        // Trim trailing newline.
        std::string top = result.stdout_output;
        while (!top.empty() && (top.back() == '\n' || top.back() == '\r')) {
            top.pop_back();
        }
        if (!top.empty()) {
            return fs::path(top);
        }
    }
    return std::nullopt;
}

bool ensure_directory(const fs::path& dir) {
    std::error_code ec;
    return fs::create_directories(dir, ec);
}

std::optional<uint64_t> file_size(const fs::path& path) {
    std::error_code ec;
    auto size = fs::file_size(path, ec);
    if (ec) {
        return std::nullopt;
    }
    return static_cast<uint64_t>(size);
}

bool is_text_file(const fs::path& path) {
    std::error_code ec;
    if (!fs::is_regular_file(path, ec)) {
        return false;
    }

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        return false;
    }

    // Read the first 8KB and check for null bytes (common binary indicator).
    std::array<char, 8192> buf;
    buf.fill(0);
    ifs.read(buf.data(), static_cast<std::streamsize>(buf.size()));
    auto n = static_cast<size_t>(ifs.gcount());

    // Files that start with a BOM or have no null bytes are likely text.
    for (size_t i = 0; i < n; ++i) {
        if (static_cast<unsigned char>(buf[i]) == 0x00) {
            return false;
        }
    }

    // Empty files are considered text.
    return true;
}

fs::path relative_path(const fs::path& base, const fs::path& target) {
    std::error_code ec;
    auto result = fs::relative(target, base, ec);
    if (ec) {
        return target;
    }
    return result;
}

}  // namespace ccmake
