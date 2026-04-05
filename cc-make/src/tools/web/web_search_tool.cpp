#include "tools/web/web_search_tool.hpp"
#include "utils/process.hpp"

#include <sstream>

namespace ccmake {

WebSearchTool::WebSearchTool() {
    def_ = make_tool_def(
        "WebSearch",
        "Search the web for information.",
        {
            {"type", "object"},
            {"properties", {
                {"query", {{"type", "string"}, {"description", "Search query"}}}
            }},
            {"required", nlohmann::json::array({"query"})}
        },
        {},
        true   // read-only
    );
}

const ToolDefinition& WebSearchTool::definition() const {
    return def_;
}

std::string WebSearchTool::validate_input(const nlohmann::json& input, const ToolContext& ctx) const {
    (void)ctx;

    if (!input.contains("query") || !input["query"].is_string()) {
        return "Missing required field: query (string)";
    }
    if (input["query"].get<std::string>().empty()) {
        return "Field 'query' must not be empty";
    }
    return "";
}

ToolOutput WebSearchTool::execute(const nlohmann::json& input, const ToolContext& ctx) {
    (void)ctx;

    auto query = input["query"].get<std::string>();

    // Use DuckDuckGo lite for simple web search (no API key needed)
    // This provides basic search capability without external dependencies
    std::ostringstream cmd;
    cmd << "curl -s -L --max-time 10 "
        << "'https://lite.duckduckgo.com/lite/?q="
        << query << "'";

    auto result = run_command(cmd.str());

    if (result.timed_out) {
        return ToolOutput{"Error: search timed out", true};
    }
    if (result.exit_code != 0) {
        return ToolOutput{"Error: " + result.stderr_output, true};
    }

    // Parse the HTML response to extract result links and snippets
    std::string body = result.stdout_output;

    // Simple extraction: find <a class="result-link"> and associated text
    std::string output;
    output += "Search results for: " + query + "\n\n";

    if (body.size() < 100) {
        return ToolOutput{output + "[No results found]", false};
    }

    // Extract links and titles using simple pattern matching
    std::vector<std::pair<std::string, std::string>> results;

    size_t pos = 0;
    while (pos < body.size()) {
        // Find result links
        auto link_start = body.find("href=\"", pos);
        if (link_start == std::string::npos) break;

        auto href_start = link_start + 6;
        auto href_end = body.find("\"", href_start);
        if (href_end == std::string::npos) break;

        auto url = body.substr(href_start, href_end - href_start);

        // Skip non-result links
        if (url.find("duckduckgo.com") != std::string::npos) {
            pos = href_end + 1;
            continue;
        }

        // Try to find associated title text
        auto title_end = body.find("</a>", href_end);
        std::string title;
        if (title_end != std::string::npos) {
            auto title_start = href_end + 2;  // skip ">"
            if (title_start < title_end) {
                title = body.substr(title_start, title_end - title_start);
            }
        }

        // Clean up HTML from title
        while (!title.empty() && (title[0] == '\n' || title[0] == '\r' || title[0] == '\t')) {
            title = title.substr(1);
        }
        while (!title.empty() && (title.back() == '\n' || title.back() == '\r' || title.back() == '\t')) {
            title.pop_back();
        }

        if (!url.empty() && url.find("http") == 0) {
            results.emplace_back(title, url);
        }

        pos = href_end + 1;
        if (results.size() >= 10) break;
    }

    if (results.empty()) {
        output += "[No results found]\n";
        output += "[Raw response length: " + std::to_string(body.size()) + " bytes]\n";
    } else {
        for (size_t i = 0; i < results.size(); ++i) {
            output += std::to_string(i + 1) + ". " + results[i].first + "\n";
            output += "   " + results[i].second + "\n\n";
        }
    }

    return ToolOutput{output, false};
}

}  // namespace ccmake
