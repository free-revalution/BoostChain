#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "utils/filesystem.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;

// Note: don't use 'using namespace ccmake' here because 'file_size' would
// clash with std::filesystem::file_size (brought in by ADL via fs::path args).

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Create a temporary directory with a few .cpp and .txt files for testing.
static fs::path make_temp_test_dir() {
    auto dir = fs::temp_directory_path() / "ccmake-test-fs-XXXXXX";
    std::string dir_str = dir.string();
    // mkdtemp modifies the string in-place.
    if (!mkdtemp(dir_str.data())) {
        throw std::runtime_error("mkdtemp failed");
    }
    return fs::path(dir_str);
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_CASE("find_files_by_extension finds matching .cpp files") {
    auto tmp = make_temp_test_dir();

    // Create some files.
    std::ofstream(tmp / "foo.cpp").put('a');
    std::ofstream(tmp / "bar.cpp").put('b');
    std::ofstream(tmp / "baz.txt").put('c');

    auto found = ccmake::find_files_by_extension(tmp, ".cpp");
    REQUIRE(found.size() == 2);

    // Cleanup.
    fs::remove_all(tmp);
}

TEST_CASE("find_files_by_extension with extension without dot") {
    auto tmp = make_temp_test_dir();

    std::ofstream(tmp / "foo.cpp").put('a');
    std::ofstream(tmp / "bar.h").put('b');

    auto found = ccmake::find_files_by_extension(tmp, "cpp");
    REQUIRE(found.size() == 1);

    fs::remove_all(tmp);
}

TEST_CASE("read_file returns file contents, nonexistent returns nullopt") {
    auto tmp = make_temp_test_dir();
    auto test_file = tmp / "test.txt";

    // Write content.
    {
        std::ofstream ofs(test_file);
        ofs << "hello world";
    }

    auto content = ccmake::read_file(test_file);
    REQUIRE(content.has_value());
    REQUIRE(*content == "hello world");

    // Nonexistent file.
    auto missing = ccmake::read_file(tmp / "does_not_exist.txt");
    REQUIRE_FALSE(missing.has_value());

    fs::remove_all(tmp);
}

TEST_CASE("is_git_repo detects current working directory as a git repo") {
    // The project itself is a git repo.
    REQUIRE(ccmake::is_git_repo(fs::current_path()));
}

TEST_CASE("is_git_repo returns false for a non-repo temp directory") {
    auto tmp = make_temp_test_dir();
    REQUIRE_FALSE(ccmake::is_git_repo(tmp));
    fs::remove_all(tmp);
}

TEST_CASE("write_file and append_file") {
    auto tmp = make_temp_test_dir();
    auto test_file = tmp / "write_test.txt";

    REQUIRE(ccmake::write_file(test_file, "line1\n"));
    auto content = ccmake::read_file(test_file);
    REQUIRE(content == "line1\n");

    REQUIRE(ccmake::append_file(test_file, "line2\n"));
    content = ccmake::read_file(test_file);
    REQUIRE(content == "line1\nline2\n");

    fs::remove_all(tmp);
}

TEST_CASE("ensure_directory creates nested directories") {
    auto tmp = make_temp_test_dir();
    auto nested = tmp / "a" / "b" / "c";

    REQUIRE(ccmake::ensure_directory(nested));
    REQUIRE(fs::is_directory(nested));

    fs::remove_all(tmp);
}

TEST_CASE("file_size returns correct size") {
    auto tmp = make_temp_test_dir();
    auto test_file = tmp / "size_test.bin";

    std::string data(1234, 'x');
    ccmake::write_file(test_file, data);

    auto size = ccmake::file_size(test_file);
    REQUIRE(size.has_value());
    REQUIRE(*size == 1234);

    // Nonexistent file.
    REQUIRE_FALSE(ccmake::file_size(tmp / "missing.bin").has_value());

    fs::remove_all(tmp);
}

TEST_CASE("is_text_file detects text vs binary") {
    auto tmp = make_temp_test_dir();

    // Text file.
    ccmake::write_file(tmp / "text.txt", "hello world\n");
    REQUIRE(ccmake::is_text_file(tmp / "text.txt"));

    // Binary file with a null byte.
    ccmake::write_file(tmp / "binary.bin", std::string("hello\0world", 11));
    REQUIRE_FALSE(ccmake::is_text_file(tmp / "binary.bin"));

    // Empty file is considered text.
    ccmake::write_file(tmp / "empty.txt", "");
    REQUIRE(ccmake::is_text_file(tmp / "empty.txt"));

    fs::remove_all(tmp);
}

TEST_CASE("relative_path computes correct relative path") {
    fs::path base = fs::current_path();
    fs::path target = base / "subdir" / "file.txt";

    auto rel = ccmake::relative_path(base, target);
    REQUIRE(rel == fs::path("subdir") / "file.txt");
}
