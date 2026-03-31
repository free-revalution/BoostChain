#ifndef BOOSTCHAIN_CHAIN_HPP
#define BOOSTCHAIN_CHAIN_HPP

#include <string>
#include <vector>
#include <memory>
#include <map>

namespace boostchain {

// Result of executing a chain or a single node
struct ChainResult {
    std::string output;                          // Primary text output
    std::map<std::string, std::string> variables; // Key-value pairs produced by this step
    bool success = true;                          // Whether execution succeeded
    std::string error;                            // Error message if success == false
};

// The Chain class orchestrates a sequence of processing nodes.
// Each node can be a prompt template, an LLM call, a tool invocation, etc.
// Nodes are executed in order, with each receiving the output of the previous.
class Chain {
public:
    Chain() = default;

    // Add a prompt template node to the chain.
    // Returns *this for method chaining.
    Chain& add_prompt(const std::string& template_str);

    // Serialize the chain to a string representation.
    std::string serialize() const;

    // Deserialize a chain from its string representation.
    static Chain deserialize(const std::string& data);

    // Execute the chain with the given input variables.
    // Each node receives the accumulated variables and may add new ones.
    ChainResult run(const std::map<std::string, std::string>& input = {}) const;

    // Get the number of nodes in the chain.
    size_t node_count() const;

public:
    // Internal node interface - must be public for implementation subclasses
    class INode;

private:
    std::vector<std::shared_ptr<INode>> nodes_;
};

} // namespace boostchain

#endif // BOOSTCHAIN_CHAIN_HPP
