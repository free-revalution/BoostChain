#pragma once

#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <map>
#include <future>
#include <boostchain/result.hpp>

namespace boostchain {

// Tool call structure (assistant requesting tool use)
struct ToolCall {
    std::string id;      // Unique ID for this tool call
    std::string name;    // Name of the function/tool to call
    std::string arguments; // JSON string of arguments
};

// Message role and content structure
struct Message {
    enum Role { user, assistant, system, tool };

    Role role;
    std::string content;
    std::optional<std::string> name;  // Optional name for the message
    std::optional<ToolCall> tool_call; // If role is tool, contains the tool call being responded to

    Message() : role(user) {}
    Message(Role r, const std::string& c) : role(r), content(c) {}
    Message(Role r, const std::string& c, const std::string& n) : role(r), content(c), name(n) {}
};

// Tool parameter definition
struct ToolParameter {
    std::string type;        // Parameter type (string, number, boolean, array, object)
    std::string description; // Human-readable description
    bool required;           // Whether parameter is required
    std::optional<std::string> enum_values; // Optional enum values as JSON array string
};

// Tool/function definition
struct ToolDefinition {
    std::string name;        // Function name
    std::string description; // Function description
    std::map<std::string, ToolParameter> parameters; // Parameters by name
};

// Chat completion request
struct ChatRequest {
    std::string model;                          // Model identifier
    std::vector<Message> messages;              // Conversation history
    std::optional<double> temperature;          // Sampling temperature (0-2)
    std::optional<double> top_p;                // Nucleus sampling threshold
    std::optional<int> max_tokens;              // Maximum tokens to generate
    std::optional<double> presence_penalty;     // Penalty for new topics (-2 to 2)
    std::optional<double> frequency_penalty;    // Penalty for repetition (-2 to 2)
    std::optional<std::vector<std::string>> stop; // Stop sequences
    std::optional<std::vector<ToolDefinition>> tools; // Available tools
    std::optional<std::string> tool_choice;     // Tool choice policy
};

// Usage statistics
struct Usage {
    int prompt_tokens;
    int completion_tokens;
    int total_tokens;
};

// Chat completion response
struct ChatResponse {
    std::string id;                  // Response ID
    std::string object;              // Object type (e.g., "chat.completion")
    int created;                     // Unix timestamp
    std::string model;               // Model used
    std::vector<Message> messages;   // Generated message(s)
    std::optional<Usage> usage;      // Token usage stats
    std::optional<std::string> finish_reason; // Reason completion finished
};

// Streaming chunk for streaming responses
struct ChatChunk {
    std::string id;                  // Chunk ID
    std::string object;              // Object type
    int created;                     // Unix timestamp
    std::string model;               // Model used
    std::vector<Message> delta;      // Incremental message updates
    std::optional<std::string> finish_reason; // Finish reason if this is final chunk
};

// Embedding request
struct EmbeddingRequest {
    std::string model;                          // Model identifier
    std::vector<std::string> inputs;            // Texts to embed
    std::optional<std::string> encoding_format; // Format (float, base64)
};

// Single embedding vector
struct Embedding {
    std::string object;      // Object type (e.g., "list")
    std::string embedding;   // Base64 or float array representation
    int index;               // Index in the input array
};

// Embedding response
struct EmbeddingResponse {
    std::string object;              // Object type
    std::vector<Embedding> data;     // Embedding vectors
    std::string model;               // Model used
    std::optional<Usage> usage;      // Token usage stats
};

// Streaming callback type
using StreamCallback = std::function<void(const ChatChunk&)>;

// Abstract LLM provider interface
class ILLMProvider {
public:
    virtual ~ILLMProvider() = default;

    // Synchronous chat completion
    // Throws: NetworkError, APIError, SerializationError
    virtual ChatResponse chat(const ChatRequest& request) = 0;

    // Safe version returning Result instead of throwing
    virtual Result<ChatResponse> chat_safe(const ChatRequest& request) = 0;

    // Asynchronous chat completion (returns future)
    // Throws: NetworkError, APIError, SerializationError
    virtual std::future<ChatResponse> chat_async(const ChatRequest& request) = 0;

    // Streaming chat completion
    // Throws: NetworkError, APIError, SerializationError
    virtual void chat_stream(const ChatRequest& request, StreamCallback callback) = 0;

    // Generate embeddings
    // Throws: NetworkError, APIError, SerializationError
    virtual EmbeddingResponse embed(const EmbeddingRequest& request) = 0;

    // Configuration methods
    virtual void set_api_key(const std::string& api_key) = 0;
    virtual void set_model(const std::string& model) = 0;
    virtual void set_base_url(const std::string& base_url) = 0;
    virtual void set_timeout(int timeout_seconds) = 0;

    // Get current configuration
    virtual std::string get_api_key() const = 0;
    virtual std::string get_model() const = 0;
    virtual std::string get_base_url() const = 0;
    virtual int get_timeout() const = 0;
};

} // namespace boostchain
