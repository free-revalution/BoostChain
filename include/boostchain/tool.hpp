#ifndef BOOSTCHAIN_TOOL_HPP
#define BOOSTCHAIN_TOOL_HPP

#include <boostchain/llm_provider.hpp>
#include <string>
#include <memory>
#include <map>
#include <vector>

namespace boostchain {

// Result of executing a tool
struct ToolResult {
    std::string content;       // Text content of the result
    bool success = true;       // Whether execution succeeded
    std::string error;         // Error message if success == false
};

// Abstract tool interface
class ITool {
public:
    virtual ~ITool() = default;

    // Get the tool definition (name, description, parameters)
    virtual ToolDefinition get_definition() const = 0;

    // Execute the tool with the given arguments JSON string
    virtual ToolResult execute(const std::string& arguments_json) = 0;

    // Get the tool name (convenience method)
    virtual std::string get_name() const {
        return get_definition().name;
    }
};

// Built-in calculator tool supporting basic arithmetic
class CalculatorTool : public ITool {
public:
    ToolDefinition get_definition() const override;
    ToolResult execute(const std::string& arguments_json) override;
};

} // namespace boostchain

#endif // BOOSTCHAIN_TOOL_HPP
