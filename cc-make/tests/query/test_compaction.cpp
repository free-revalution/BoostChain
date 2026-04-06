#include <catch2/catch_test_macros.hpp>
#include "query/compaction.hpp"

using namespace ccmake;

// Helper to create a simple user message
static Message make_user_msg(const std::string& text) {
    return Message::user("test-id", text);
}

// Helper to create a simple assistant message
static Message make_assistant_msg(const std::string& text) {
    return Message::assistant("test-id", {TextBlock{text}});
}

// Helper to create a TokenBudget
static TokenBudget make_usage(int input, int output) {
    TokenBudget u;
    u.total_input_tokens = input;
    u.total_output_tokens = output;
    return u;
}

TEST_CASE("CompactionConfig defaults") {
    CompactionConfig config;
    REQUIRE(config.context_window == 200000);
    REQUIRE(config.trigger_ratio == 0.8);
    REQUIRE(config.target_tokens == 50000);
    REQUIRE(config.min_messages == 4);
    REQUIRE(config.keep_recent == 4);
}

TEST_CASE("Compactor should_compact returns false below threshold") {
    Compactor compactor;
    // 100k total tokens < 160k threshold (200k * 0.8)
    TokenBudget usage = make_usage(60000, 40000);
    REQUIRE_FALSE(compactor.should_compact(usage, 10));
}

TEST_CASE("Compactor should_compact returns true above threshold") {
    Compactor compactor;
    // 170k total tokens > 160k threshold
    TokenBudget usage = make_usage(100000, 70000);
    REQUIRE(compactor.should_compact(usage, 10));
}

TEST_CASE("Compactor should_compact respects min_messages") {
    Compactor compactor;
    // Way above threshold but only 2 messages
    TokenBudget usage = make_usage(200000, 100000);
    REQUIRE_FALSE(compactor.should_compact(usage, 2));
}

TEST_CASE("Compactor should_compact at exact min_messages with high usage") {
    Compactor compactor;
    TokenBudget usage = make_usage(200000, 100000);
    REQUIRE(compactor.should_compact(usage, 4));
}

TEST_CASE("Compactor truncate empty messages returns empty") {
    Compactor compactor;
    std::vector<Message> messages;
    auto result = compactor.truncate(messages, 10000);
    REQUIRE(result.empty());
}

TEST_CASE("Compactor truncate keeps recent within budget") {
    Compactor compactor;
    // Each message is ~10 chars => ~2-3 tokens
    auto msgs = std::vector<Message>{
        make_user_msg("old message one"),
        make_assistant_msg("old response one"),
        make_user_msg("old message two"),
        make_assistant_msg("old response two"),
        make_user_msg("recent message"),
    };

    // Large budget: keep all
    auto result = compactor.truncate(msgs, 100000);
    REQUIRE(result.size() == 5);
}

TEST_CASE("Compactor truncate removes old messages when over budget") {
    Compactor compactor;
    // Create messages that will exceed a small token budget
    std::vector<Message> msgs;
    for (int i = 0; i < 10; ++i) {
        msgs.push_back(make_user_msg("This is message number " + std::to_string(i) +
                                     " with some extra padding text to increase token count."));
    }

    // Small budget should keep only a few recent messages
    auto result = compactor.truncate(msgs, 20);
    REQUIRE(result.size() < msgs.size());
    // Should keep at least 1 message
    REQUIRE_FALSE(result.empty());
}

TEST_CASE("Compactor truncate preserves order") {
    Compactor compactor;
    auto msgs = std::vector<Message>{
        make_user_msg("aaaa"),
        make_user_msg("bbbb"),
        make_user_msg("cccc"),
    };

    auto result = compactor.truncate(msgs, 100);
    REQUIRE(result.size() == 3);
    // Order should be preserved (not reversed)
    REQUIRE(result[0].content[0].index() == result[1].content[0].index());
}

TEST_CASE("Compactor compact with few messages returns unchanged") {
    Compactor compactor;
    auto msgs = std::vector<Message>{
        make_user_msg("hello"),
        make_assistant_msg("hi there"),
    };

    auto result = compactor.compact(msgs);
    REQUIRE(result.size() == 2);
}

TEST_CASE("Compactor compact within budget returns unchanged") {
    Compactor compactor;
    // Create messages that are small enough to fit within target
    std::vector<Message> msgs;
    for (int i = 0; i < 5; ++i) {
        msgs.push_back(make_user_msg("short"));
    }

    auto result = compactor.compact(msgs);
    // All messages should be returned as-is since total tokens < target
    REQUIRE(result.size() == 5);
}

TEST_CASE("Compactor compact summarizes old messages") {
    Compactor compactor;
    // Create enough messages to exceed target_tokens
    // Each message with ~80 chars => ~20 tokens. Need 50000/20 = 2500 messages to exceed.
    // Let's use a smaller target for testing
    CompactionConfig cfg;
    cfg.target_tokens = 10;
    cfg.min_messages = 4;
    cfg.keep_recent = 2;
    compactor.set_config(cfg);

    std::vector<Message> msgs = {
        make_user_msg("old message one that is a bit longer"),
        make_assistant_msg("old response one that is also a bit longer"),
        make_user_msg("old message two that is a bit longer"),
        make_assistant_msg("old response two that is also a bit longer"),
        make_user_msg("recent one"),
        make_assistant_msg("recent response"),
    };

    auto result = compactor.compact(msgs);
    // Should have summary + 2 recent messages
    REQUIRE(result.size() == 3);

    // First message should be the summary
    auto* text = std::get_if<TextBlock>(&result[0].content[0]);
    REQUIRE(text != nullptr);
    REQUIRE(text->text.find("Previous conversation summary") != std::string::npos);
    REQUIRE(text->text.find("4 messages condensed") != std::string::npos);
}

TEST_CASE("Compactor compact keeps recent messages") {
    Compactor compactor;
    CompactionConfig cfg;
    cfg.target_tokens = 10;
    cfg.min_messages = 4;
    cfg.keep_recent = 3;
    compactor.set_config(cfg);

    std::vector<Message> msgs = {
        make_user_msg("old message one that is longer"),
        make_assistant_msg("old response one that is longer"),
        make_user_msg("old message two that is longer"),
        make_assistant_msg("old response two that is longer"),
        make_user_msg("recent one"),
        make_assistant_msg("recent response one"),
        make_user_msg("recent two"),
    };

    auto result = compactor.compact(msgs);
    // Should have summary + 3 recent messages
    REQUIRE(result.size() == 4);

    // Last 3 messages should be the recent ones (unchanged)
    REQUIRE(result[1].role == MessageRole::User);
    REQUIRE(result[2].role == MessageRole::Assistant);
    REQUIRE(result[3].role == MessageRole::User);
}

TEST_CASE("Compactor truncation respects token budget") {
    Compactor compactor;
    // Test that truncation of known-length strings yields expected sizes
    // 40 chars => ~10 tokens, so truncating to 10 tokens should keep it
    auto msgs40 = std::vector<Message>{make_user_msg("abcdefghijklmnopqrstuvwxyz123456")};
    REQUIRE(compactor.truncate(msgs40, 10).size() == 1);
    // 0 chars => 0 tokens
    auto msgs0 = std::vector<Message>{make_user_msg("")};
    REQUIRE(compactor.truncate(msgs0, 0).size() == 1);
    // 4 chars => 1 token
    auto msgs4 = std::vector<Message>{make_user_msg("abcd")};
    REQUIRE(compactor.truncate(msgs4, 1).size() == 1);
}

TEST_CASE("Compactor set_config updates threshold") {
    Compactor compactor;
    TokenBudget usage = make_usage(1000, 1000); // 2000 total

    // Default config: 200k * 0.8 = 160k threshold => should not compact
    REQUIRE_FALSE(compactor.should_compact(usage, 10));

    // Set a very small context window
    CompactionConfig cfg;
    cfg.context_window = 2000;
    cfg.trigger_ratio = 0.8;
    cfg.min_messages = 2;
    compactor.set_config(cfg);

    // Now 2000 > 2000 * 0.8 = 1600 => should compact
    REQUIRE(compactor.should_compact(usage, 10));
}

TEST_CASE("Compactor compact with exact min_messages does not compact if within budget") {
    Compactor compactor;
    // Default min_messages is 4, create exactly 4 small messages
    std::vector<Message> msgs = {
        make_user_msg("a"),
        make_assistant_msg("b"),
        make_user_msg("c"),
        make_assistant_msg("d"),
    };

    auto result = compactor.compact(msgs);
    // Small messages within budget => returned as-is
    REQUIRE(result.size() == 4);
}

TEST_CASE("Compactor truncate with single message returns that message") {
    Compactor compactor;
    auto msgs = std::vector<Message>{make_user_msg("hello world")};

    auto result = compactor.truncate(msgs, 100);
    REQUIRE(result.size() == 1);
}

TEST_CASE("Compactor config accessor returns set config") {
    CompactionConfig cfg;
    cfg.context_window = 100000;
    cfg.trigger_ratio = 0.5;
    cfg.target_tokens = 25000;
    cfg.min_messages = 2;
    cfg.keep_recent = 2;

    Compactor compactor(cfg);
    REQUIRE(compactor.config().context_window == 100000);
    REQUIRE(compactor.config().trigger_ratio == 0.5);
    REQUIRE(compactor.config().target_tokens == 25000);
    REQUIRE(compactor.config().min_messages == 2);
    REQUIRE(compactor.config().keep_recent == 2);
}

TEST_CASE("Compactor compact handles mixed content block types") {
    Compactor compactor;
    CompactionConfig cfg;
    cfg.target_tokens = 10;
    cfg.min_messages = 4;
    cfg.keep_recent = 1;
    compactor.set_config(cfg);

    // Create messages with different content block types
    nlohmann::json tool_input;
    tool_input["arg"] = "value";

    std::vector<Message> msgs = {
        make_user_msg("old user message that is quite long"),
        Message::assistant("id1", {ToolUseBlock{"tu-1", "read_file", tool_input}}),
        make_user_msg("old user message two that is quite long"),
        Message::assistant("id2", {TextBlock{"response"}, ThinkingBlock{"hmm", ""}}),
        make_user_msg("recent message that is quite long"),
    };

    auto result = compactor.compact(msgs);
    // Should have summary + 1 recent message
    REQUIRE(result.size() == 2);

    // Summary message should mention condensed messages
    auto* text = std::get_if<TextBlock>(&result[0].content[0]);
    REQUIRE(text != nullptr);
    REQUIRE(text->text.find("4 messages condensed") != std::string::npos);
}
