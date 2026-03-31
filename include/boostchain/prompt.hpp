#ifndef BOOSTCHAIN_PROMPT_HPP
#define BOOSTCHAIN_PROMPT_HPP

#include <string>
#include <map>
#include <regex>
#include <stdexcept>

namespace boostchain {

class ConfigError : public std::runtime_error {
public:
    explicit ConfigError(const std::string& msg) : std::runtime_error(msg) {}
};

using Variables = std::map<std::string, std::string>;

class PromptTemplate {
public:
    explicit PromptTemplate(const std::string& template_str);

    std::string render(const Variables& vars) const;

private:
    std::string template_str_;
    std::regex var_pattern_;

    static std::regex create_var_pattern();
};

} // namespace boostchain

#endif // BOOSTCHAIN_PROMPT_HPP
