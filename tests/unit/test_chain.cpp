#include <boostchain/chain.hpp>
#include <boostchain/prompt.hpp>
#include <cassert>

using namespace boostchain;

void test_chain_add_prompt() {
    Chain chain;
    chain.add_prompt("Hello, {{name}}!");

    // Should be able to serialize
    std::string serialized = chain.serialize();
    assert(!serialized.empty());
    assert(serialized.find("prompt") != std::string::npos);
}

void test_chain_deserialize() {
    Chain chain;
    chain.add_prompt("Hello, {{name}}!");

    std::string serialized = chain.serialize();
    Chain deserialized = Chain::deserialize(serialized);

    std::string reserialized = deserialized.serialize();
    assert(serialized == reserialized);
}

void test_chain_multiple_prompts() {
    Chain chain;
    chain.add_prompt("Step 1: {{input}}")
         .add_prompt("Step 2: Process {{input}}");

    std::string serialized = chain.serialize();
    // Should contain both prompts
    assert(serialized.find("Step 1") != std::string::npos);
    assert(serialized.find("Step 2") != std::string::npos);
}

int main() {
    test_chain_add_prompt();
    test_chain_deserialize();
    test_chain_multiple_prompts();
    return 0;
}
