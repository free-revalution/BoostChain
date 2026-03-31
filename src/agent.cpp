#include <boostchain/agent.hpp>
#include <boostchain/error.hpp>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cstdio>

namespace boostchain {

Agent::Agent(std::shared_ptr<ILLMProvider> llm)
    : llm_(std::move(llm)) {
    if (!llm_) {
        throw ConfigError("Agent requires a non-null LLM provider");
    }
}

Agent::Agent(std::shared_ptr<ILLMProvider> llm,
             std::vector<std::shared_ptr<ITool>> tools)
    : llm_(std::move(llm)), tools_(std::move(tools)) {
    if (!llm_) {
        throw ConfigError("Agent requires a non-null LLM provider");
    }
}

void Agent::set_system_prompt(const std::string& prompt) {
    system_prompt_ = prompt;
}

void Agent::reset() {
    conversation_history_.clear();
}

void Agent::set_max_iterations(int max_iterations) {
    max_iterations_ = max_iterations;
}

std::vector<Message> Agent::build_messages(const std::string& user_input) const {
    std::vector<Message> messages;

    if (!system_prompt_.empty()) {
        messages.push_back(Message(Message::system, system_prompt_));
    }

    messages.insert(messages.end(),
                    conversation_history_.begin(),
                    conversation_history_.end());

    messages.push_back(Message(Message::user, user_input));

    return messages;
}

Message Agent::execute_tool_call(const ToolCall& tool_call) {
    // Find the tool by name
    for (auto& tool : tools_) {
        if (tool->get_name() == tool_call.name) {
            ToolResult result = tool->execute(tool_call.arguments);

            Message msg(Message::tool, result.success ? result.content : result.error);
            msg.name = tool_call.name;
            msg.tool_call = tool_call;
            return msg;
        }
    }

    // Tool not found
    Message msg(Message::tool,
                "Error: Unknown tool '" + tool_call.name + "'");
    msg.name = tool_call.name;
    msg.tool_call = tool_call;
    return msg;
}

AgentResult Agent::run(const std::string& user_input) {
    if (!tools_.empty()) {
        return run_with_tools(user_input);
    }

    // Original behavior without tools
    AgentResult result;

    if (!llm_) {
        result.completed = false;
        result.steps_taken = 0;
        result.final_answer = "Error: LLM provider is null";
        return result;
    }

    auto messages = build_messages(user_input);

    try {
        ChatRequest request;
        request.model = llm_->get_model();
        request.messages = std::move(messages);

        ChatResponse response = llm_->chat(request);

        if (response.messages.empty()) {
            result.completed = false;
            result.steps_taken = 0;
            result.final_answer = "Error: LLM returned empty response";
            return result;
        }

        const Message& assistant_msg = response.messages.back();
        result.final_answer = assistant_msg.content;
        result.steps_taken = 1;
        result.completed = true;

        conversation_history_.push_back(Message(Message::user, user_input));
        conversation_history_.push_back(assistant_msg);

    } catch (const std::exception& e) {
        result.completed = false;
        result.steps_taken = 0;
        result.final_answer = std::string("Error: ") + e.what();
    }

    return result;
}

AgentResult Agent::run_with_tools(const std::string& user_input) {
    AgentResult result;

    if (!llm_) {
        result.completed = false;
        result.steps_taken = 0;
        result.final_answer = "Error: LLM provider is null";
        return result;
    }

    auto messages = build_messages(user_input);

    // Add tool definitions to the request
    std::vector<ToolDefinition> tool_defs;
    for (const auto& tool : tools_) {
        tool_defs.push_back(tool->get_definition());
    }

    int steps = 0;

    try {
        while (steps < max_iterations_) {
            ChatRequest request;
            request.model = llm_->get_model();
            request.messages = messages;
            request.tools = tool_defs;

            ChatResponse response = llm_->chat(request);

            if (response.messages.empty()) {
                result.completed = false;
                result.steps_taken = steps;
                result.final_answer = "Error: LLM returned empty response";
                return result;
            }

            steps++;
            const Message& last_msg = response.messages.back();

            // Check if the assistant wants to call tools
            bool has_tool_calls = false;

            // Look through response messages for tool calls
            for (const auto& resp_msg : response.messages) {
                messages.push_back(resp_msg);
                if (resp_msg.tool_call.has_value()) {
                    has_tool_calls = true;
                }
            }

            if (!has_tool_calls) {
                // No tool calls - this is the final answer
                result.final_answer = last_msg.content;
                result.steps_taken = steps;
                result.completed = true;
                break;
            }

            // Execute tool calls
            // Find all tool calls from the last assistant message block
            for (size_t i = response.messages.size(); i > 0; --i) {
                const Message& resp_msg = response.messages[i - 1];
                if (resp_msg.tool_call.has_value()) {
                    Message tool_result = execute_tool_call(resp_msg.tool_call.value());
                    messages.push_back(tool_result);
                }
            }
        }

        if (steps >= max_iterations_) {
            result.completed = false;
            result.steps_taken = steps;
            result.final_answer = "Error: Agent exceeded maximum iterations ("
                                  + std::to_string(max_iterations_) + ")";
        }

    } catch (const std::exception& e) {
        result.completed = false;
        result.steps_taken = steps;
        result.final_answer = std::string("Error: ") + e.what();
    }

    // Update conversation history with only the NEW messages from this run.
    // messages = [system_prompt?, ...old_history..., user_input, ...new_llm/tool_msgs...]
    // We skip the system prompt (index 0) and the old history (indices 1..history_size).
    size_t history_size = conversation_history_.size();
    // messages layout: [system?] [old_history_0..old_history_{n-1}] [user_input] [new_messages...]
    size_t user_msg_index = system_prompt_.empty() ? 0 : 1 + history_size;
    for (size_t i = user_msg_index; i < messages.size(); ++i) {
        conversation_history_.push_back(messages[i]);
    }

    return result;
}

// ============================================================================
// Persistence
// ============================================================================

// Helper: JSON-escape a string for save_state output
static std::string json_escape(const std::string& s) {
    std::string result;
    result.reserve(s.size() + 16);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n";  break;
            case '\t': result += "\\t";  break;
            case '\r': result += "\\r";  break;
            case '\b': result += "\\b";  break;
            case '\f': result += "\\f";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    result += buf;
                } else {
                    result += static_cast<char>(c);
                }
                break;
        }
    }
    return result;
}

std::string Agent::save_state() const {
    std::ostringstream oss;

    oss << "{\n";
    oss << "  \"system_prompt\": \"" << json_escape(system_prompt_) << "\",\n";

    oss << "  \"conversation_history\": [\n";
    for (size_t i = 0; i < conversation_history_.size(); ++i) {
        const Message& msg = conversation_history_[i];
        oss << "    {\"role\": ";

        switch (msg.role) {
            case Message::user: oss << "\"user\""; break;
            case Message::assistant: oss << "\"assistant\""; break;
            case Message::system: oss << "\"system\""; break;
            case Message::tool: oss << "\"tool\""; break;
        }

        oss << ", \"content\": \"" << json_escape(msg.content) << "\"";

        if (msg.name.has_value()) {
            oss << ", \"name\": \"" << json_escape(msg.name.value()) << "\"";
        }

        oss << "}";
        if (i + 1 < conversation_history_.size()) oss << ",";
        oss << "\n";
    }
    oss << "  ]\n";
    oss << "}\n";

    return oss.str();
}

// Minimal JSON parser for load_state
struct JsonReader {
    const std::string& json;
    size_t pos = 0;

    explicit JsonReader(const std::string& j) : json(j) {}

    void skip_ws() {
        while (pos < json.size() &&
               (json[pos] == ' ' || json[pos] == '\n' ||
                json[pos] == '\r' || json[pos] == '\t')) {
            pos++;
        }
    }

    char peek() {
        skip_ws();
        if (pos < json.size()) return json[pos];
        return '\0';
    }

    void expect(char c) {
        skip_ws();
        if (pos < json.size() && json[pos] == c) {
            pos++;
        }
    }

    bool match_char(char c) {
        skip_ws();
        if (pos < json.size() && json[pos] == c) {
            pos++;
            return true;
        }
        return false;
    }

    std::string read_string() {
        skip_ws();
        if (pos >= json.size() || json[pos] != '"') return "";
        pos++; // skip opening quote
        std::string result;
        while (pos < json.size()) {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                char next = json[pos + 1];
                switch (next) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case 'n': result += '\n'; break;
                    case 't': result += '\t'; break;
                    case 'r': result += '\r'; break;
                    default: result += next;
                }
                pos += 2;
            } else if (json[pos] == '"') {
                pos++; // skip closing quote
                break;
            } else {
                result += json[pos];
                pos++;
            }
        }
        return result;
    }

    std::string read_object_value_string(const std::string& key) {
        // Expect to be inside an object, search for the key
        std::string search = "\"" + key + "\"";
        size_t key_pos = json.find(search, pos);
        if (key_pos == std::string::npos) return "";

        pos = key_pos + search.size();
        skip_ws();
        expect(':');
        skip_ws();
        return read_string();
    }

    void skip_object() {
        skip_ws();
        if (pos >= json.size() || json[pos] != '{') return;
        pos++; // skip '{'
        int depth = 1;
        while (pos < json.size() && depth > 0) {
            if (json[pos] == '{') depth++;
            else if (json[pos] == '}') depth--;
            else if (json[pos] == '"') {
                pos++;
                while (pos < json.size() && json[pos] != '"') {
                    if (json[pos] == '\\') pos++;
                    pos++;
                }
            }
            if (depth > 0) pos++;
        }
        if (pos < json.size()) pos++; // skip final '}'
    }

    void skip_array() {
        skip_ws();
        if (pos >= json.size() || json[pos] != '[') return;
        pos++; // skip '['
        int depth = 1;
        while (pos < json.size() && depth > 0) {
            if (json[pos] == '[') depth++;
            else if (json[pos] == ']') depth--;
            else if (json[pos] == '"') {
                pos++;
                while (pos < json.size() && json[pos] != '"') {
                    if (json[pos] == '\\') pos++;
                    pos++;
                }
            }
            if (depth > 0) pos++;
        }
        if (pos < json.size()) pos++; // skip final ']'
    }
};

} // anonymous namespace

namespace boostchain {

bool Agent::load_state(const std::string& json_data) {
    try {
        if (json_data.empty()) return false;

        JsonReader reader(json_data);
        reader.skip_ws();
        if (reader.peek() != '{') return false;

        // Read system_prompt
        std::string sys_prompt = reader.read_object_value_string("system_prompt");
        system_prompt_ = sys_prompt;

        // Find conversation_history array
        std::string search = "\"conversation_history\"";
        size_t arr_pos = json_data.find(search, reader.pos);
        if (arr_pos == std::string::npos) return false;

        reader.pos = arr_pos + search.size();
        reader.skip_ws();
        reader.expect(':');
        reader.skip_ws();
        if (reader.peek() != '[') return false;
        reader.pos++; // skip '['

        conversation_history_.clear();

        while (reader.peek() != ']' && reader.pos < json_data.size()) {
            reader.skip_ws();
            if (reader.peek() != '{') break;
            reader.pos++; // skip '{'

            Message msg;

            // Read role
            std::string role_str = reader.read_object_value_string("role");
            if (role_str == "user") msg.role = Message::user;
            else if (role_str == "assistant") msg.role = Message::assistant;
            else if (role_str == "system") msg.role = Message::system;
            else if (role_str == "tool") msg.role = Message::tool;
            else msg.role = Message::user;

            // Read content
            std::string content = reader.read_object_value_string("content");
            msg.content = content;

            // Read optional name
            // Try to find "name" field
            size_t name_pos = json_data.find("\"name\"", reader.pos);
            if (name_pos != std::string::npos) {
                // Check if it's before the closing brace of this object
                size_t close_brace = json_data.find('}', reader.pos);
                if (close_brace != std::string::npos && name_pos < close_brace) {
                    reader.pos = name_pos + 6; // length of "\"name\""
                    reader.skip_ws();
                    reader.expect(':');
                    reader.skip_ws();
                    if (reader.peek() == '"') {
                        msg.name = reader.read_string();
                    }
                }
            }

            // Skip to end of this object
            reader.skip_ws();
            while (reader.pos < json_data.size() && json_data[reader.pos] != '}') {
                if (json_data[reader.pos] == '"') {
                    reader.pos++;
                    while (reader.pos < json_data.size() && json_data[reader.pos] != '"') {
                        if (json_data[reader.pos] == '\\') reader.pos++;
                        reader.pos++;
                    }
                    if (reader.pos < json_data.size()) reader.pos++; // closing quote
                } else {
                    reader.pos++;
                }
            }
            if (reader.pos < json_data.size()) reader.pos++; // skip '}'

            conversation_history_.push_back(msg);

            reader.skip_ws();
            if (reader.peek() == ',') reader.pos++; // skip comma
        }

        return true;
    } catch (...) {
        return false;
    }
}

} // namespace boostchain
