#ifndef BOOSTCHAIN_PROMPT_HPP
#define BOOSTCHAIN_PROMPT_HPP

#include <boostchain/error.hpp>
#include <string>
#include <map>

namespace boostchain {

using Variables = std::map<std::string, std::string>;

class PromptTemplate {
public:
    explicit PromptTemplate(const std::string& template_str);

    std::string render(const Variables& vars) const;

private:
    std::string template_str_;
};

} // namespace boostchain

#endif // BOOSTCHAIN_PROMPT_HPP
