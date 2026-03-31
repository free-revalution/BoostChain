#include <boostchain/llm/openai_compatible.hpp>
#include <boostchain/error.hpp>
#include <cassert>
#include <iostream>
#include <cstdlib>

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

    try {
        auto response = provider->chat(req);

        assert(!response.messages.empty());
        assert(!response.messages[0].content.empty());
        std::cout << "DeepSeek: " << response.messages[0].content << "\n";
    } catch (const boostchain::Error& e) {
        std::cerr << "TEST ERROR: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "UNEXPECTED ERROR: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
