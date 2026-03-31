#include <boostchain/prompt.hpp>
#include <set>

namespace boostchain {

PromptTemplate::PromptTemplate(const std::string& template_str)
    : template_str_(template_str) {
}

namespace {

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

} // anonymous namespace

std::string PromptTemplate::render(const Variables& vars) const {
    // First pass: collect variable names and check for missing ones
    std::set<std::string> seen;
    std::set<std::string> required;
    size_t pos = 0;
    while (pos < template_str_.size()) {
        size_t open_pos = template_str_.find("{{", pos);
        if (open_pos == std::string::npos) break;
        size_t close_pos = template_str_.find("}}", open_pos);
        if (close_pos == std::string::npos) break;
        std::string var_name = trim(template_str_.substr(open_pos + 2, close_pos - open_pos - 2));
        if (!var_name.empty()) {
            required.insert(var_name);
        }
        pos = close_pos + 2;
    }

    for (const auto& name : required) {
        if (vars.find(name) == vars.end()) {
            throw ConfigError("Missing variable: " + name);
        }
    }

    // Single-pass replacement: build result incrementally
    std::string result;
    pos = 0;
    while (pos < template_str_.size()) {
        size_t open_pos = template_str_.find("{{", pos);
        if (open_pos == std::string::npos) {
            result.append(template_str_, pos);
            break;
        }

        // Append text before {{var}}
        result.append(template_str_, pos, open_pos - pos);

        size_t close_pos = template_str_.find("}}", open_pos);
        if (close_pos == std::string::npos) {
            result.append(template_str_, open_pos);
            break;
        }

        // Extract and trim variable name
        std::string var_name = trim(template_str_.substr(open_pos + 2, close_pos - open_pos - 2));

        // Lookup and append replacement
        auto it = vars.find(var_name);
        if (it != vars.end()) {
            result.append(it->second);
        }

        pos = close_pos + 2;
    }

    return result;
}

} // namespace boostchain
