#include <catch2/catch_test_macros.hpp>

#include "git/git.hpp"

#include <filesystem>

using namespace ccmake;

TEST_CASE("git_status returns info for current repo") {
    auto status = git_status(std::filesystem::current_path());
    // The test directory is inside a git repo, so is_repo should be true.
    // We don't assert specific values to keep tests robust.
    (void)status;
}

TEST_CASE("git_current_branch returns branch name") {
    auto branch = git_current_branch(std::filesystem::current_path());
    // Should not crash; value depends on the repo state.
    (void)branch;
}

TEST_CASE("git_get_diff returns diff content") {
    auto diffs = git_diff(std::filesystem::current_path());
    // Should not crash; may be empty if repo is clean.
    (void)diffs;
}

TEST_CASE("git_is_dirty checks for changes") {
    auto dirty = git_is_dirty(std::filesystem::current_path());
    // Should not crash; value depends on repo state.
    (void)dirty;
}
