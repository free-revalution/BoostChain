#include <catch2/catch_test_macros.hpp>
#include "api/claude_client.hpp"
#include "core/types.hpp"
#include <nlohmann/json.hpp>

using namespace ccmake;

TEST_CASE("ClaudeClientConfig defaults") {
    ClaudeClientConfig config;
    REQUIRE(config.model.empty());
    REQUIRE(config.max_tokens == 8192);
    REQUIRE(config.max_retries == 10);
}

TEST_CASE("ClaudeClientConfig custom") {
    ClaudeClientConfig config;
    config.model = "claude-sonnet-4-20250514";
    config.max_tokens = 16384;
    config.system_prompt = "Helpful.";
    REQUIRE(config.model == "claude-sonnet-4-20250514");
}

TEST_CASE("StreamSession accumulates text") {
    StreamSession s;
    s.on_content_block_start(StreamContentBlockStart{0, APIBlockStartText{""}});
    s.on_content_block_delta(StreamContentBlockDelta{0, APIDeltaText{"Hello"}});
    s.on_content_block_delta(StreamContentBlockDelta{0, APIDeltaText{" world"}});
    s.on_content_block_stop(StreamContentBlockStop{0});
    REQUIRE(s.content_blocks.size() == 1);
    auto* t = std::get_if<TextBlock>(&s.content_blocks[0]);
    REQUIRE(t != nullptr);
    REQUIRE(t->text == "Hello world");
}

TEST_CASE("StreamSession accumulates tool use") {
    StreamSession s;
    s.on_content_block_start(StreamContentBlockStart{0, APIBlockStartToolUse{"toolu_1", "Read", ""}});
    s.on_content_block_delta(StreamContentBlockDelta{0, APIDeltaToolInput{"{\"path\":"}});
    s.on_content_block_delta(StreamContentBlockDelta{0, APIDeltaToolInput{"\"/tmp\""}});
    s.on_content_block_delta(StreamContentBlockDelta{0, APIDeltaToolInput{"}"}}});
    s.on_content_block_stop(StreamContentBlockStop{0});
    REQUIRE(s.content_blocks.size() == 1);
    auto* tool = std::get_if<ToolUseBlock>(&s.content_blocks[0]);
    REQUIRE(tool != nullptr);
    REQUIRE(tool->name == "Read");
    REQUIRE(tool->id == "toolu_1");
    REQUIRE(tool->input["path"] == "/tmp");
}

TEST_CASE("StreamSession accumulates thinking") {
    StreamSession s;
    s.on_content_block_start(StreamContentBlockStart{0, APIBlockStartThinking{"", ""}});
    s.on_content_block_delta(StreamContentBlockDelta{0, APIDeltaThinking{"Let me think..."}});
    s.on_content_block_delta(StreamContentBlockDelta{0, APIDeltaSignature{"sig-abc"}});
    s.on_content_block_stop(StreamContentBlockStop{0});
    REQUIRE(s.content_blocks.size() == 1);
    auto* th = std::get_if<ThinkingBlock>(&s.content_blocks[0]);
    REQUIRE(th != nullptr);
    REQUIRE(th->thinking == "Let me think...");
    REQUIRE(th->signature == "sig-abc");
}

TEST_CASE("StreamSession tracks usage from message_start") {
    StreamSession s;
    StreamMessageStart start;
    start.id = "msg_1";
    start.usage.input_tokens = 100;
    s.on_message_start(start);
    REQUIRE(s.usage.total_input_tokens == 100);
    REQUIRE(s.message_id == "msg_1");
}

TEST_CASE("StreamSession updates usage from message_delta") {
    StreamSession s;
    StreamMessageDelta delta;
    delta.usage.output_tokens = 42;
    delta.stop_reason = "end_turn";
    s.on_message_delta(delta);
    REQUIRE(s.usage.total_output_tokens == 42);
    REQUIRE(s.stop_reason == StopReason::EndTurn);
}

TEST_CASE("StreamSession maps stop reasons") {
    StreamSession s;
    StreamMessageDelta d;
    d.stop_reason = "tool_use";
    s.on_message_delta(d);
    REQUIRE(s.stop_reason == StopReason::ToolUse);

    StreamSession s2;
    StreamMessageDelta d2;
    d2.stop_reason = "max_tokens";
    s2.on_message_delta(d2);
    REQUIRE(s2.stop_reason == StopReason::MaxTokens);
}

TEST_CASE("StreamSession handles interleaved blocks") {
    StreamSession s;
    s.on_content_block_start(StreamContentBlockStart{0, APIBlockStartThinking{"", ""}});
    s.on_content_block_delta(StreamContentBlockDelta{0, APIDeltaThinking{"hmm"}});
    s.on_content_block_stop(StreamContentBlockStop{0});
    s.on_content_block_start(StreamContentBlockStart{1, APIBlockStartText{""}});
    s.on_content_block_delta(StreamContentBlockDelta{1, APIDeltaText{"I'll read it."}});
    s.on_content_block_stop(StreamContentBlockStop{1});
    s.on_content_block_start(StreamContentBlockStart{2, APIBlockStartToolUse{"toolu_1", "Read", ""}});
    s.on_content_block_delta(StreamContentBlockDelta{2, APIDeltaToolInput{"{\"path\":\"/f\"}"}});
    s.on_content_block_stop(StreamContentBlockStop{2});
    REQUIRE(s.content_blocks.size() == 3);
    REQUIRE(std::holds_alternative<ThinkingBlock>(s.content_blocks[0]));
    REQUIRE(std::holds_alternative<TextBlock>(s.content_blocks[1]));
    REQUIRE(std::holds_alternative<ToolUseBlock>(s.content_blocks[2]));
}

TEST_CASE("StreamSession builds complete Message") {
    StreamSession s;
    s.on_message_start(StreamMessageStart{"msg_1", "claude-sonnet", {100, 0}});
    s.on_content_block_start(StreamContentBlockStart{0, APIBlockStartText{""}});
    s.on_content_block_delta(StreamContentBlockDelta{0, APIDeltaText{"Hello!"}});
    s.on_content_block_stop(StreamContentBlockStop{0});
    StreamMessageDelta d;
    d.stop_reason = "end_turn";
    d.usage.output_tokens = 5;
    s.on_message_delta(d);
    auto msg = s.build_message();
    REQUIRE(msg.role == MessageRole::Assistant);
    REQUIRE(msg.id == "msg_1");
    REQUIRE(msg.content.size() == 1);
    REQUIRE(msg.stop_reason == StopReason::EndTurn);
    REQUIRE(msg.usage.total_output_tokens == 5);
}

TEST_CASE("ClaudeClient construction") {
    ClaudeClientConfig config;
    config.model = "claude-sonnet-4-20250514";
    config.api_key = "sk-test-key";
    ClaudeClient client(config);
    REQUIRE(client.transport().config().api_key == "sk-test-key");
}
