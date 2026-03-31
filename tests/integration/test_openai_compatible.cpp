#include <boostchain/llm/openai_compatible.hpp>
#include <cassert>
#include <iostream>

using namespace boostchain;

int main() {
    const char* api_key = std::getenv("DEEPSEEK_API_KEY");
    if (!api_key) {
        std::cout << "SKIP: DEEPSEEK_API_KEY not set\n";
        return 0;
    }

    auto provider = std::make_shared<OpenAICompatibleProvider>(
        "https://api.deepseek.com/v1",
        api_key
    );
    provider->set_model("deepseek-chat");

    ChatRequest req;
    req.messages = {{Message::user, "Hello! What is 2+2?"}};

    auto response = provider->chat(req);

    assert(!response.messages.empty());
    assert(!response.messages[0].content.empty());
    std::cout << "DeepSeek: " << response.messages[0].content << "\n";

    return 0;
}
