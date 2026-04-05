#include <catch2/catch_test_macros.hpp>
#include "session/session.hpp"
#include <filesystem>
#include <fstream>
using namespace ccmake;
namespace fs = std::filesystem;

static fs::path make_test_dir() {
    static int counter = 0;
    auto dir = fs::path("/tmp") / ("ccmake_session_" + std::to_string(counter++));
    fs::create_directories(dir);
    return dir;
}

// ============================================================
// SessionStore basic operations
// ============================================================

TEST_CASE("SessionStore create_session") {
    auto dir = make_test_dir();
    SessionStore store(dir);
    auto id = store.create_session("test-model", "/tmp");

    REQUIRE_FALSE(id.empty());
    REQUIRE(fs::is_directory(dir / id));

    auto meta = store.load_meta(id);
    REQUIRE(meta.has_value());
    REQUIRE(meta->id == id);
    REQUIRE(meta->model == "test-model");

    fs::remove_all(dir);
}

TEST_CASE("SessionStore save and load messages") {
    auto dir = make_test_dir();
    SessionStore store(dir);
    auto id = store.create_session("model-a", "/home");

    std::vector<Message> messages;
    messages.push_back(Message::user("u1", "Hello"));
    messages.push_back(Message::assistant("a1", {TextBlock{"Hi there!"}}));

    REQUIRE(store.save_session(id, messages, "model-a", "/home"));

    auto loaded = store.load_session(id);
    REQUIRE(loaded.has_value());
    REQUIRE(loaded->size() == 2);
    REQUIRE(loaded->at(0).role == MessageRole::User);
    REQUIRE(loaded->at(1).role == MessageRole::Assistant);

    fs::remove_all(dir);
}

TEST_CASE("SessionStore save and load tool use messages") {
    auto dir = make_test_dir();
    SessionStore store(dir);
    auto id = store.create_session("model", "/tmp");

    std::vector<Message> messages;
    messages.push_back(Message::user("u1", "Read the file"));
    messages.push_back(Message::assistant("a1", {ToolUseBlock{"t1", "Read", {{"file_path", "/tmp/test.txt"}}}}));
    messages.push_back(Message::assistant("a2", {ToolResultBlock{"t1", "file content here", false}}));

    REQUIRE(store.save_session(id, messages, "model", "/tmp"));

    auto loaded = store.load_session(id);
    REQUIRE(loaded.has_value());
    REQUIRE(loaded->size() == 3);

    auto* tu = std::get_if<ToolUseBlock>(&loaded->at(1).content[0]);
    REQUIRE(tu != nullptr);
    REQUIRE(tu->name == "Read");
    REQUIRE(tu->input["file_path"] == "/tmp/test.txt");

    auto* tr = std::get_if<ToolResultBlock>(&loaded->at(2).content[0]);
    REQUIRE(tr != nullptr);
    REQUIRE(tr->content == "file content here");
    REQUIRE_FALSE(tr->is_error);

    fs::remove_all(dir);
}

TEST_CASE("SessionStore load nonexistent session") {
    auto dir = make_test_dir();
    SessionStore store(dir);
    REQUIRE_FALSE(store.load_session("nonexistent").has_value());
    fs::remove_all(dir);
}

TEST_CASE("SessionStore save nonexistent session") {
    auto dir = make_test_dir();
    SessionStore store(dir);
    REQUIRE_FALSE(store.save_session("nonexistent", {}, "model", "/tmp"));
    fs::remove_all(dir);
}

// ============================================================
// SessionStore list and delete
// ============================================================

TEST_CASE("SessionStore list_sessions") {
    auto dir = make_test_dir();
    SessionStore store(dir);
    store.create_session("model-a", "/tmp");
    store.create_session("model-b", "/tmp");
    store.create_session("model-c", "/tmp");

    REQUIRE(store.list_sessions().size() == 3);
    fs::remove_all(dir);
}

TEST_CASE("SessionStore list_sessions empty") {
    auto dir = make_test_dir();
    SessionStore store(dir);
    REQUIRE(store.list_sessions().empty());
    fs::remove_all(dir);
}

TEST_CASE("SessionStore delete_session") {
    auto dir = make_test_dir();
    SessionStore store(dir);
    auto id = store.create_session("model", "/tmp");

    REQUIRE(store.delete_session(id));
    REQUIRE_FALSE(fs::exists(dir / id));
    REQUIRE(store.list_sessions().empty());
    fs::remove_all(dir);
}

TEST_CASE("SessionStore delete nonexistent session") {
    auto dir = make_test_dir();
    SessionStore store(dir);
    REQUIRE_FALSE(store.delete_session("nonexistent"));
    fs::remove_all(dir);
}

TEST_CASE("SessionStore save updates message count") {
    auto dir = make_test_dir();
    SessionStore store(dir);
    auto id = store.create_session("model", "/tmp");

    std::vector<Message> messages;
    messages.push_back(Message::user("u1", "Hello"));
    messages.push_back(Message::assistant("a1", {TextBlock{"Hi"}}));
    messages.push_back(Message::user("u2", "Bye"));

    store.save_session(id, messages, "model", "/tmp");

    auto meta = store.load_meta(id);
    REQUIRE(meta.has_value());
    REQUIRE(meta->message_count == 3);
    fs::remove_all(dir);
}

TEST_CASE("SessionStore save and load thinking blocks") {
    auto dir = make_test_dir();
    SessionStore store(dir);
    auto id = store.create_session("model", "/tmp");

    std::vector<Message> messages;
    messages.push_back(Message::assistant("a1", {ThinkingBlock{"Let me think...", "sig123"}}));

    REQUIRE(store.save_session(id, messages, "model", "/tmp"));

    auto loaded = store.load_session(id);
    REQUIRE(loaded.has_value());
    auto* th = std::get_if<ThinkingBlock>(&loaded->at(0).content[0]);
    REQUIRE(th != nullptr);
    REQUIRE(th->thinking == "Let me think...");
    REQUIRE(th->signature == "sig123");
    fs::remove_all(dir);
}

TEST_CASE("SessionStore base_dir") {
    auto dir = make_test_dir();
    SessionStore store(dir);
    REQUIRE(store.base_dir() == dir);
    fs::remove_all(dir);
}
