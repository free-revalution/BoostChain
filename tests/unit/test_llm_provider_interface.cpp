#include <boostchain/llm_provider.hpp>
#include <cassert>
#include <memory>

using namespace boostchain;

void test_message_construction() {
    Message msg;
    msg.role = Message::user;
    msg.content = "Hello";
    assert(msg.role == Message::user);
    assert(msg.content == "Hello");
    assert(!msg.tool_call.has_value());
}

void test_tool_call_structure() {
    ToolCall call;
    call.id = "call_123";
    call.name = "calculator";
    call.arguments = R"({"expression":"2+2"})";
    assert(call.name == "calculator");
    assert(call.arguments.find("2+2") != std::string::npos);
}

void test_chat_request_structure() {
    ChatRequest req;
    req.model = "gpt-4";
    req.temperature = 0.7;
    req.messages.push_back({Message::user, "Hello"});
    assert(req.model == "gpt-4");
    assert(req.messages.size() == 1);
}

void test_tool_definition() {
    ToolDefinition tool;
    tool.name = "http_request";
    tool.description = "Make HTTP requests";
    tool.parameters["url"] = ToolParameter{"string", "Request URL", true, std::nullopt};
    assert(tool.name == "http_request");
    assert(tool.parameters.at("url").required == true);
}

int main() {
    test_message_construction();
    test_tool_call_structure();
    test_chat_request_structure();
    test_tool_definition();
    return 0;
}
