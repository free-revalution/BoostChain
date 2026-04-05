#include "tools/web/web_fetch_tool.hpp"
#include "utils/process.hpp"

#include <algorithm>
#include <regex>
#include <sstream>

namespace ccmake {

WebFetchTool::WebFetchTool() {
    def_ = make_tool_def(
        "WebFetch",
        "Fetch content from a URL and convert to readable text.",
        {
            {"type", "object"},
            {"properties", {
                {"url", {{"type", "string"}, {"description", "The URL to fetch"}}},
                {"max_chars", {{"type", "integer"}, {"description", "Maximum characters to return (default: 50000)"}}}
            }},
            {"required", nlohmann::json::array({"url"})}
        },
        {},
        true   // read-only
    );
}

const ToolDefinition& WebFetchTool::definition() const {
    return def_;
}

std::string WebFetchTool::validate_input(const nlohmann::json& input, const ToolContext& ctx) const {
    (void)ctx;

    if (!input.contains("url") || !input["url"].is_string()) {
        return "Missing required field: url (string)";
    }
    auto url = input["url"].get<std::string>();
    if (url.empty()) {
        return "Field 'url' must not be empty";
    }
    if (url.find("://") == std::string::npos) {
        return "Field 'url' must include a protocol (http:// or https://)";
    }
    return "";
}

std::string WebFetchTool::decode_html_entities(const std::string& text) {
    std::string result;
    result.reserve(text.size());

    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '&') {
            auto semi = text.find(';', i);
            if (semi != std::string::npos && semi - i < 10) {
                std::string entity = text.substr(i, semi - i + 1);
                if (entity == "&amp;") { result += '&'; i = semi; continue; }
                if (entity == "&lt;") { result += '<'; i = semi; continue; }
                if (entity == "&gt;") { result += '>'; i = semi; continue; }
                if (entity == "&quot;") { result += '"'; i = semi; continue; }
                if (entity == "&apos;") { result += '\''; i = semi; continue; }
                if (entity == "&#39;") { result += '\''; i = semi; continue; }
                if (entity == "&nbsp;") { result += ' '; i = semi; continue; }
                // Numeric entities: &#NNN;
                if (entity.size() > 3 && entity[1] == '#') {
                    int code = std::stoi(entity.substr(2, semi - i - 2));
                    if (code > 0 && code < 65536) {
                        result += static_cast<char>(code);
                        i = semi;
                        continue;
                    }
                }
            }
        }
        result += text[i];
    }
    return result;
}

std::string WebFetchTool::strip_html_tags(const std::string& html) {
    std::string result;
    result.reserve(html.size());

    bool in_script = false;
    bool in_style = false;
    bool in_tag = false;

    for (size_t i = 0; i < html.size(); ++i) {
        if (in_tag) {
            if (html[i] == '>') {
                in_tag = false;
                // Check what tag we just closed
                // For simplicity, we check the opening tag below
            }
            continue;
        }

        if (html[i] == '<') {
            // Check for script/style tags
            if (i + 7 < html.size() && html.substr(i, 7) == "<script") {
                in_script = true;
                continue;
            }
            if (i + 6 < html.size() && html.substr(i, 6) == "<style") {
                in_style = true;
                continue;
            }
            if (in_script && html.substr(i, 9) == "</script>") {
                in_script = false;
                i += 8;
                continue;
            }
            if (in_style && html.substr(i, 7) == "</style>") {
                in_style = false;
                i += 6;
                continue;
            }
            if (!in_script && !in_style) {
                in_tag = true;
                // Convert block-level tags to newlines
                auto remaining = html.substr(i);
                if (remaining.find("<br") == 0 || remaining.find("<BR") == 0 ||
                    remaining.find("<p") == 0 || remaining.find("<P") == 0 ||
                    remaining.find("<div") == 0 || remaining.find("<DIV") == 0 ||
                    remaining.find("<h1") == 0 || remaining.find("<h2") == 0 ||
                    remaining.find("<h3") == 0 || remaining.find("<h4") == 0 ||
                    remaining.find("<h5") == 0 || remaining.find("<h6") == 0 ||
                    remaining.find("<li") == 0 || remaining.find("<LI") == 0 ||
                    remaining.find("<tr") == 0 || remaining.find("<TR") == 0) {
                    if (!result.empty() && result.back() != '\n') {
                        result += '\n';
                    }
                }
                continue;
            }
        }

        if (!in_script && !in_style) {
            result += html[i];
        }
    }

    // Decode HTML entities
    result = decode_html_entities(result);

    // Collapse multiple whitespace/newlines
    std::string cleaned;
    cleaned.reserve(result.size());
    bool last_was_space = false;
    bool last_was_newline = false;

    for (char c : result) {
        if (c == '\n') {
            if (!last_was_newline) {
                cleaned += c;
                last_was_newline = true;
                last_was_space = false;
            }
        } else if (c == ' ' || c == '\t' || c == '\r') {
            if (!last_was_space && !last_was_newline) {
                cleaned += ' ';
                last_was_space = true;
            }
        } else {
            cleaned += c;
            last_was_space = false;
            last_was_newline = false;
        }
    }

    return cleaned;
}

ToolOutput WebFetchTool::execute(const nlohmann::json& input, const ToolContext& ctx) {
    (void)ctx;

    auto url = input["url"].get<std::string>();
    int max_chars = 50000;
    if (input.contains("max_chars") && input["max_chars"].is_number_integer()) {
        max_chars = input["max_chars"].get<int>();
    }

    std::string cmd = "curl -s -L --max-time 15 '" + url + "'";

    auto result = run_command(cmd);

    if (result.timed_out) {
        return ToolOutput{"Error: request timed out", true};
    }
    if (result.exit_code != 0) {
        return ToolOutput{"Error: " + result.stderr_output, true};
    }

    std::string body = result.stdout_output;

    // Check content type — if HTML, strip tags
    if (body.find("<!DOCTYPE") != std::string::npos ||
        body.find("<html") != std::string::npos ||
        body.find("<HTML") != std::string::npos) {
        body = strip_html_tags(body);
    }

    // Truncate if too long
    if (static_cast<int>(body.size()) > max_chars) {
        body = body.substr(0, max_chars) + "\n\n[Content truncated at " +
               std::to_string(max_chars) + " characters]";
    }

    return ToolOutput{body, false};
}

}  // namespace ccmake
