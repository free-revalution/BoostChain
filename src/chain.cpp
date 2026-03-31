#include <boostchain/chain.hpp>
#include <boostchain/prompt.hpp>
#include <boostchain/error.hpp>
#include <boostchain/llm_provider.hpp>
#include <sstream>
#include <stdexcept>

namespace boostchain {

// Forward declaration of JSON helper
std::string unescape_json(const std::string& s);

// ============================================================================
// Internal node interface
// ============================================================================

class Chain::INode {
public:
    virtual ~INode() = default;

    // Execute this node with the current accumulated variables.
    // Returns a ChainResult with updated output and variables.
    virtual ChainResult execute(const std::map<std::string, std::string>& vars) const = 0;

    // Serialize this node to a string representation.
    virtual std::string serialize() const = 0;

    // Get the type name of this node (for deserialization dispatch).
    virtual std::string type_name() const = 0;
};

// ============================================================================
// PromptNode - renders a prompt template with the current variables
// ============================================================================

class PromptNode : public Chain::INode {
public:
    explicit PromptNode(const std::string& template_str)
        : template_str_(template_str) {}

    ChainResult execute(const std::map<std::string, std::string>& vars) const override {
        ChainResult result;
        result.success = true;

        try {
            // Use PromptTemplate if variables are provided
            if (!vars.empty()) {
                // Convert to Variables map
                Variables prompt_vars;
                for (const auto& kv : vars) {
                    prompt_vars[kv.first] = kv.second;
                }
                PromptTemplate tpl(template_str_);
                result.output = tpl.render(prompt_vars);
            } else {
                // No variables - just use the template as-is
                // Check if the template has any {{var}} patterns
                PromptTemplate tpl(template_str_);
                Variables prompt_vars;
                // Pass through empty vars - if template has no vars, it renders as-is
                // If it has vars, render() will throw, which we catch
                result.output = tpl.render(prompt_vars);
            }
        } catch (const boostchain::ConfigError& e) {
            // Template has required variables that are missing
            // Return the raw template string as output
            result.output = template_str_;
        }

        // Propagate all input variables through
        result.variables = vars;
        return result;
    }

    std::string serialize() const override {
        std::ostringstream oss;
        oss << "{\"type\":\"prompt\",\"template\":\""
            << escape_json(template_str_) << "\"}";
        return oss.str();
    }

    std::string type_name() const override {
        return "prompt";
    }

    const std::string& template_str() const {
        return template_str_;
    }

private:
    static std::string escape_json(const std::string& s) {
        std::string result;
        result.reserve(s.size() + 16);
        for (char c : s) {
            switch (c) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n";  break;
                case '\r': result += "\\r";  break;
                case '\t': result += "\\t";  break;
                default:   result += c;      break;
            }
        }
        return result;
    }

    std::string template_str_;
};

// ============================================================================
// LLMNode - placeholder for future LLM integration
// ============================================================================

class LLMNode : public Chain::INode {
public:
    explicit LLMNode(std::shared_ptr<ILLMProvider> provider)
        : provider_(std::move(provider)) {}

    ChainResult execute(const std::map<std::string, std::string>& vars) const override {
        ChainResult result;
        result.variables = vars;

        if (!provider_) {
            result.success = false;
            result.error = "LLM provider is null";
            result.output = "";
            return result;
        }

        try {
            ChatRequest request;
            request.model = provider_->get_model();

            // Build messages from accumulated variables
            // Use "last_output" if present, otherwise combine all variables
            std::string input;
            auto it = vars.find("last_output");
            if (it != vars.end()) {
                input = it->second;
            } else {
                for (const auto& kv : vars) {
                    if (!input.empty()) input += "\n";
                    input += kv.first + ": " + kv.second;
                }
            }

            if (!input.empty()) {
                request.messages.push_back(Message(Message::user, input));
            }

            ChatResponse response = provider_->chat(request);

            if (!response.messages.empty()) {
                result.output = response.messages.back().content;
            } else {
                result.output = "";
            }
            result.success = true;

            // Store the output for downstream nodes
            result.variables["last_output"] = result.output;
        } catch (const std::exception& e) {
            result.success = false;
            result.error = std::string("LLM call failed: ") + e.what();
            result.output = "";
        }

        return result;
    }

    std::string serialize() const override {
        std::ostringstream oss;
        oss << "{\"type\":\"llm\"";
        if (provider_) {
            oss << ",\"model\":\"" << provider_->get_model() << "\"";
        }
        oss << "}";
        return oss.str();
    }

    std::string type_name() const override {
        return "llm";
    }

private:
    std::shared_ptr<ILLMProvider> provider_;
};

// ============================================================================
// Chain implementation
// ============================================================================

Chain& Chain::add_prompt(const std::string& template_str) {
    nodes_.push_back(std::make_shared<PromptNode>(template_str));
    return *this;
}

Chain& Chain::add_llm(std::shared_ptr<ILLMProvider> provider) {
    nodes_.push_back(std::make_shared<LLMNode>(std::move(provider)));
    return *this;
}

std::string Chain::serialize() const {
    std::ostringstream oss;
    oss << "{\"nodes\":[";
    for (size_t i = 0; i < nodes_.size(); ++i) {
        if (i > 0) oss << ",";
        oss << nodes_[i]->serialize();
    }
    oss << "]}";
    return oss.str();
}

Chain Chain::deserialize(const std::string& data) {
    // Simple JSON parsing for our known format
    // Format: {"nodes":[{"type":"prompt","template":"..."}, ...]}

    Chain chain;

    // Find the nodes array
    auto nodes_start = data.find("\"nodes\":[");
    if (nodes_start == std::string::npos) {
        throw SerializationError("Invalid chain data: missing 'nodes' array");
    }
    nodes_start += 9; // skip "nodes":[

    // Find the closing bracket for the nodes array
    size_t depth = 1;
    auto nodes_end = nodes_start;
    while (nodes_end < data.size() && depth > 0) {
        if (data[nodes_end] == '[') depth++;
        else if (data[nodes_end] == ']') depth--;
        if (depth > 0) nodes_end++;
    }

    if (depth != 0) {
        throw SerializationError("Invalid chain data: malformed nodes array");
    }

    std::string nodes_str = data.substr(nodes_start, nodes_end - nodes_start);

    // Parse individual node objects
    // Each node is {"type":"...","template":"..."} or {"type":"llm","model":"..."}
    size_t pos = 0;
    while (pos < nodes_str.size()) {
        // Skip whitespace
        while (pos < nodes_str.size() && (nodes_str[pos] == ' ' || nodes_str[pos] == '\n' || nodes_str[pos] == '\r' || nodes_str[pos] == '\t' || nodes_str[pos] == ',')) {
            pos++;
        }
        if (pos >= nodes_str.size()) break;

        // Find the opening brace of this node
        if (nodes_str[pos] != '{') {
            pos++;
            continue;
        }

        // Find matching closing brace
        size_t brace_depth = 0;
        size_t node_end = pos;
        bool in_string = false;
        while (node_end < nodes_str.size()) {
            char c = nodes_str[node_end];
            if (c == '"' && (node_end == 0 || nodes_str[node_end - 1] != '\\')) {
                in_string = !in_string;
            }
            if (!in_string) {
                if (c == '{') brace_depth++;
                else if (c == '}') {
                    brace_depth--;
                    if (brace_depth == 0) {
                        node_end++;
                        break;
                    }
                }
            }
            node_end++;
        }

        std::string node_str = nodes_str.substr(pos, node_end - pos);

        // Extract type field
        auto type_pos = node_str.find("\"type\"");
        if (type_pos == std::string::npos) {
            pos = node_end;
            continue;
        }

        auto type_colon = node_str.find(":", type_pos);
        auto type_quote_start = node_str.find("\"", type_colon);
        auto type_quote_end = node_str.find("\"", type_quote_start + 1);
        std::string type = node_str.substr(type_quote_start + 1, type_quote_end - type_quote_start - 1);

        if (type == "prompt") {
            // Extract template field
            auto tpl_pos = node_str.find("\"template\"");
            if (tpl_pos != std::string::npos) {
                auto tpl_colon = node_str.find(":", tpl_pos);
                auto tpl_quote_start = node_str.find("\"", tpl_colon);
                auto tpl_quote_end = tpl_quote_start + 1;

                // Find the closing quote (handling escaped quotes)
                while (tpl_quote_end < node_str.size()) {
                    if (node_str[tpl_quote_end] == '"' && node_str[tpl_quote_end - 1] != '\\') {
                        break;
                    }
                    tpl_quote_end++;
                }

                std::string tpl_value = unescape_json(
                    node_str.substr(tpl_quote_start + 1, tpl_quote_end - tpl_quote_start - 1));
                chain.add_prompt(tpl_value);
            }
        }
        // LLM nodes can be added here in the future

        pos = node_end;
    }

    return chain;
}

ChainResult Chain::run(const std::map<std::string, std::string>& input) const {
    ChainResult result;
    std::map<std::string, std::string> accumulated = input;
    std::string last_output;

    for (const auto& node : nodes_) {
        result = node->execute(accumulated);
        if (!result.success) {
            // Stop on first failure
            return result;
        }
        // Update accumulated variables with what the node produced
        for (const auto& kv : result.variables) {
            accumulated[kv.first] = kv.second;
        }
        last_output = result.output;
    }

    // Final result
    result.output = last_output;
    result.variables = accumulated;
    return result;
}

size_t Chain::node_count() const {
    return nodes_.size();
}

// ============================================================================
// JSON helpers (namespace scope)
// ============================================================================

std::string unescape_json(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char next = s[i + 1];
            switch (next) {
                case '"':  result += '"';  ++i; break;
                case '\\': result += '\\'; ++i; break;
                case 'n':  result += '\n'; ++i; break;
                case 'r':  result += '\r'; ++i; break;
                case 't':  result += '\t'; ++i; break;
                default:   result += s[i]; break;
            }
        } else {
            result += s[i];
        }
    }
    return result;
}

} // namespace boostchain
