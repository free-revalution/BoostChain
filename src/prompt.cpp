#include <boostchain/prompt.hpp>

namespace boostchain {

PromptTemplate::PromptTemplate(const std::string& template_str)
    : template_str_(template_str)
    , var_pattern_(create_var_pattern()) {
}

std::regex PromptTemplate::create_var_pattern() {
    return std::regex(R"(\{\{([^}]+)\}\})");
}

std::string PromptTemplate::render(const Variables& vars) const {
    std::string result = template_str_;
    std::sregex_iterator it(result.begin(), result.end(), var_pattern_);
    std::sregex_iterator end;

    // First pass: check for missing variables
    for (std::sregex_iterator i = it; i != end; ++i) {
        std::smatch match = *i;
        std::string var_name = match[1].str();

        // Trim whitespace from variable name
        size_t start = var_name.find_first_not_of(" \t\n\r");
        size_t end = var_name.find_last_not_of(" \t\n\r");

        if (start == std::string::npos) {
            throw ConfigError("Empty variable name in template");
        }

        std::string trimmed_name = var_name.substr(start, end - start + 1);

        if (vars.find(trimmed_name) == vars.end()) {
            throw ConfigError("Missing variable: " + trimmed_name);
        }
    }

    // Second pass: replace variables
    it = std::sregex_iterator(result.begin(), result.end(), var_pattern_);
    size_t pos = 0;

    while (it != end) {
        std::smatch match = *it;
        std::string var_name = match[1].str();

        // Trim whitespace from variable name
        size_t start = var_name.find_first_not_of(" \t\n\r");
        size_t end = var_name.find_last_not_of(" \t\n\r");
        std::string trimmed_name = var_name.substr(start, end - start + 1);

        auto var_it = vars.find(trimmed_name);
        if (var_it != vars.end()) {
            result.replace(match.position(), match.length(), var_it->second);
            // Update iterator to continue from after the replacement
            it = std::sregex_iterator(result.begin(), result.end(), var_pattern_);
        } else {
            ++it;
        }
    }

    return result;
}

} // namespace boostchain
