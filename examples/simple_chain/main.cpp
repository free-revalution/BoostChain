#include <boostchain/boostchain.hpp>
#include <boostchain/chain.hpp>
#include <boostchain/llm/openai_provider.hpp>
#include <iostream>

int main() {
    const char* api_key = std::getenv("OPENAI_API_KEY");
    if (!api_key) {
        std::cerr << "Please set OPENAI_API_KEY\n";
        return 1;
    }

    auto llm = std::make_shared<boostchain::OpenAIProvider>(api_key);
    llm->set_model("gpt-3.5-turbo");

    boostchain::Chain chain;
    chain.add_prompt("You are a helpful assistant. Answer concisely.")
         .add_llm(llm);

    auto result = chain.run({{"user_input", "What is BoostChain?"}});
    std::cout << "Response: " << result.output << "\n";
    return 0;
}
