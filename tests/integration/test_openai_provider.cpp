#include <boostchain/llm/openai_provider.hpp>
#include <boostchain/error.hpp>
#include <cassert>
#include <iostream>
#include <cstdlib>

using namespace boostchain;

int main() {
    const char* api_key = std::getenv("OPENAI_API_KEY");
    if (!api_key) {
        std::cout << "SKIP: OPENAI_API_KEY not set\n";
        return 0;
    }

    OpenAIProvider provider(api_key);
    provider.set_model("gpt-3.5-turbo");

    ChatRequest req;
    req.model = "gpt-3.5-turbo";
    req.messages = {{Message::user, "Say 'Hello, BoostChain!'"}};

    try {
        auto response = provider.chat(req);

        assert(!response.messages.empty());
        assert(!response.messages[0].content.empty());
        std::cout << "Response: " << response.messages[0].content << "\n";
    } catch (const boostchain::Error& e) {
        std::cerr << "TEST ERROR: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "UNEXPECTED ERROR: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
