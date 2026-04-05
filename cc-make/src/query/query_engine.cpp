#include "query/query_engine.hpp"
#include "query/agentic_loop.hpp"
#include "query/context.hpp"
#include "config/config.hpp"

#include <spdlog/spdlog.h>

namespace ccmake {

QueryEngine::QueryEngine(const std::string& model) : model_(model) {}

std::string QueryEngine::model() const { return model_; }
void QueryEngine::set_model(const std::string& model) { model_ = model; }
std::string QueryEngine::system_prompt() const { return system_prompt_; }
void QueryEngine::set_system_prompt(const std::string& prompt) { system_prompt_ = prompt; }
void QueryEngine::set_max_turns(int max_turns) { max_turns_ = max_turns; }
void QueryEngine::set_max_tokens(int64_t max_tokens) { max_tokens_ = max_tokens; }
void QueryEngine::set_thinking(const std::optional<APIThinkingConfig>& thinking) { thinking_ = thinking; }
void QueryEngine::set_cwd(const std::string& cwd) { cwd_ = cwd; }

void QueryEngine::register_tool(const std::string& name, ToolExecuteFunction fn) {
    tool_executor_.register_tool(name, fn);
}

bool QueryEngine::has_tool(const std::string& name) const {
    return tool_executor_.has_tool(name) || tool_registry_.has(name);
}

bool QueryEngine::register_tool(std::unique_ptr<ToolBase> tool) {
    if (!tool) return false;
    const auto& def = tool->definition();
    auto tool_name = def.name;

    // Bridge: register a ToolExecuteFunction that calls the tool
    auto tool_ptr = tool.get();  // capture raw pointer (owned by registry)
    tool_executor_.register_tool(tool_name,
        [tool_ptr](const std::string& name, const nlohmann::json& input) -> nlohmann::json {
            ToolContext ctx;
            auto output = tool_ptr->execute(input, ctx);
            if (output.is_error) {
                throw std::runtime_error(output.content);
            }
            // Return the content as JSON string
            return output.content;
        }
    );

    return tool_registry_.register_tool(std::move(tool));
}

ToolRegistry& QueryEngine::tool_registry() { return tool_registry_; }
const ToolRegistry& QueryEngine::tool_registry() const { return tool_registry_; }

PermissionManager& QueryEngine::permission_manager() { return permission_manager_; }
const PermissionManager& QueryEngine::permission_manager() const { return permission_manager_; }
void QueryEngine::set_permission_mode(PermissionMode mode) { permission_manager_.set_mode(mode); }

const std::vector<Message>& QueryEngine::messages() const { return messages_; }

void QueryEngine::interrupt() { abort_signal_.abort(); }

void QueryEngine::set_session_id(const std::string& id) { session_id_ = id; }
std::string QueryEngine::session_id() const { return session_id_; }
void QueryEngine::enable_auto_save(bool enabled) { auto_save_ = enabled; }

void QueryEngine::set_mock_response(const Message& response) {
    mock_responses_ = {response};
    mock_index_ = 0;
    use_mock_ = true;
}

void QueryEngine::set_mock_responses(const std::vector<Message>& responses) {
    mock_responses_ = responses;
    mock_index_ = 0;
    use_mock_ = true;
}

Message QueryEngine::next_mock_response() {
    if (mock_responses_.empty()) {
        return Message::assistant("mock", {TextBlock{"(no mock response)"}});
    }
    auto msg = mock_responses_[mock_index_ % mock_responses_.size()];
    mock_index_++;
    return msg;
}

TurnResult QueryEngine::submit_message(const std::string& prompt) {
    abort_signal_.reset();

    // Wire permission manager to tool executor
    permission_manager_.context();  // ensure initialized
    tool_executor_.set_permission_manager(&permission_manager_);

    // Build context
    std::string cwd = cwd_.empty() ? "." : cwd_;
    auto user_context = build_user_context(cwd);
    auto system_context = build_system_context(cwd);
    std::string full_prompt = build_system_prompt(
        QueryConfig{model_, system_prompt_}, user_context, system_context
    );

    // Build API call function
    auto api_call = [this](
        const std::vector<Message>& /*messages*/,
        const std::string& /*system_prompt*/,
        const std::vector<APIToolDefinition>& /*tools*/,
        QueryEventCallback /*on_event*/,
        const std::vector<APIToolDefinition>& /*api_tools*/
    ) -> Message {
        if (use_mock_) {
            return next_mock_response();
        }
        // Real API call would go through ClaudeClient here
        return Message::assistant("a", {TextBlock{"(not connected)"}});
    };

    // Configure agentic loop
    AgenticLoopConfig loop_config;
    loop_config.max_turns = max_turns_;
    loop_config.abort_signal = &abort_signal_;
    loop_config.system_prompt = full_prompt;
    loop_config.tool_executor = &tool_executor_;
    loop_config.model = model_;
    loop_config.max_tokens = max_tokens_;
    loop_config.thinking = thinking_;

    // Create user message
    Message user_msg = Message::user("u_" + std::to_string(messages_.size()), prompt);

    // Run the loop with existing message history
    auto result = run_agentic_loop(api_call, loop_config, user_msg, messages_);

    // Update message history
    messages_ = result.messages;

    // Auto-save session
    if (auto_save_ && !session_id_.empty()) {
        try {
            SessionStore store(get_global_config_dir() / "sessions");
            store.save_session(session_id_, messages_, model_, cwd_);
        } catch (...) {
            // Don't let save failures crash the engine
        }
    }

    return result;
}

}  // namespace ccmake
