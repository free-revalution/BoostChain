#include "query/agentic_loop.hpp"

namespace ccmake {

TurnResult run_agentic_loop(
    APICallFunction api_call,
    const AgenticLoopConfig& config,
    const Message& user_message
) {
    TurnResult result;
    LoopState state;

    state.messages.push_back(user_message);

    auto check_abort = [&]() -> bool {
        return config.abort_signal && config.abort_signal->is_aborted();
    };

    while (true) {
        if (check_abort()) {
            result.exit_reason = LoopExitReason::Aborted;
            break;
        }

        if (state.turn_count >= config.max_turns) {
            result.exit_reason = LoopExitReason::MaxTurns;
            break;
        }

        state.turn_count++;

        // Call API
        Message assistant_msg;
        try {
            assistant_msg = api_call(
                state.messages,
                config.system_prompt,
                config.tools,
                config.on_event,
                config.tools
            );
        } catch (const std::exception& e) {
            result.exit_reason = LoopExitReason::ModelError;
            result.error_message = e.what();
            break;
        }

        // Add assistant message to history
        state.messages.push_back(assistant_msg);

        // Check abort after API call
        if (check_abort()) {
            result.exit_reason = LoopExitReason::Aborted;
            break;
        }

        // Collect tool use blocks from the assistant response
        state.collect_tool_uses(assistant_msg);

        // No tool use -> done
        if (!state.needs_follow_up()) {
            result.exit_reason = LoopExitReason::Completed;
            break;
        }

        // Execute tools
        Message tool_result_msg;
        tool_result_msg.id = "tr_" + std::to_string(state.turn_count);
        tool_result_msg.role = MessageRole::User;

        if (config.tool_executor) {
            std::vector<ToolExecutor::PendingTool> pending;
            for (const auto& tu : state.pending_tool_use_blocks) {
                pending.push_back({tu.id, tu.name, tu.input});
            }

            auto tool_results = config.tool_executor->execute(pending);

            for (const auto& tr : tool_results) {
                tool_result_msg.content.push_back(ToolResultBlock{
                    tr.tool_use_id, tr.content, tr.is_error
                });

                if (config.on_event) {
                    config.on_event(QueryEventToolResult{
                        tr.tool_use_id, tr.content, tr.is_error
                    });
                }
            }
        } else {
            // No tool executor — synthesize error results
            for (const auto& tu : state.pending_tool_use_blocks) {
                tool_result_msg.content.push_back(ToolResultBlock{
                    tu.id, "Error: no tool executor configured", true
                });
            }
        }

        state.messages.push_back(tool_result_msg);

        // Clear pending tool use blocks for next iteration
        state.pending_tool_use_blocks.clear();
    }

    result.messages = std::move(state.messages);
    result.total_usage = state.total_usage;
    return result;
}

}  // namespace ccmake
