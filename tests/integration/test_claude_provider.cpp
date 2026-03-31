#include <boostchain/llm/claude_provider.hpp>
#include <cassert>
#include <iostream>

using namespace boostchain;

int main() {
    const char* api_key = std::getenv("ANTHROPIC_API_KEY");
    if (!api_key) {
        std::cout << "SKIP: ANTHROPIC_API_KEY not set\n";
        return 0;
    }

    ClaudeProvider provider(api_key);
    provider.set_model("claude-3-5-sonnet-20241022");

    // Test configuration
    assert(provider.get_model() == "claude-3-5-sonnet-20241022");
    assert(provider.get_base_url() == "https://api.anthropic.com/v1");

    ChatRequest req;
    req.model = "claude-3-5-sonnet-20241022";
    req.messages = {{Message::user, "Say 'Hello, BoostChain!' in one line."}};
    req.max_tokens = 100;

    auto response = provider.chat(req);

    assert(!response.messages.empty());
    assert(!response.messages[0].content.empty());
    std::cout << "Claude Response: " << response.messages[0].content << "\n";

    // Test with system message
    ChatRequest req2;
    req2.model = "claude-3-5-sonnet-20241022";
    req2.messages = {
        {Message::system, "You are a helpful assistant that only speaks in French."},
        {Message::user, "Say hello."}
    };
    req2.max_tokens = 100;

    auto response2 = provider.chat(req2);
    assert(!response2.messages.empty());
    std::cout << "Claude System Response: " << response2.messages[0].content << "\n";

    std::cout << "All Claude provider tests PASSED!\n";
    return 0;
}
