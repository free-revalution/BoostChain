#include <boostchain/llm/gemini_provider.hpp>
#include <boostchain/error.hpp>
#include <cassert>
#include <iostream>
#include <cstdlib>

using namespace boostchain;

int main() {
    const char* api_key = std::getenv("GEMINI_API_KEY");
    if (!api_key) {
        std::cout << "SKIP: GEMINI_API_KEY not set\n";
        return 0;
    }

    GeminiProvider provider(api_key);
    provider.set_model("gemini-pro");

    // Test configuration
    assert(provider.get_model() == "gemini-pro");
    assert(provider.get_base_url() == "https://generativelanguage.googleapis.com/v1beta");

    ChatRequest req;
    req.model = "gemini-pro";
    req.messages = {{Message::user, "Say 'Hello, BoostChain!' in one line."}};

    try {
        auto response = provider.chat(req);

        assert(!response.messages.empty());
        assert(!response.messages[0].content.empty());
        std::cout << "Gemini Response: " << response.messages[0].content << "\n";
    } catch (const boostchain::Error& e) {
        std::cerr << "TEST ERROR: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "UNEXPECTED ERROR: " << e.what() << "\n";
        return 1;
    }

    std::cout << "All Gemini provider tests PASSED!\n";
    return 0;
}
