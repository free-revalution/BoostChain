#include <boostchain/prompt.hpp>
#include <cassert>
#include <map>

using namespace boostchain;

void test_simple_template() {
    PromptTemplate tpl("Hello, {{name}}!");
    Variables vars;
    vars["name"] = "World";

    std::string result = tpl.render(vars);
    assert(result == "Hello, World!");
}

void test_multiple_variables() {
    PromptTemplate tpl("{{greeting}}, {{name}}! Today is {{day}}.");
    Variables vars;
    vars["greeting"] = "Hello";
    vars["name"] = "Alice";
    vars["day"] = "Monday";

    std::string result = tpl.render(vars);
    assert(result == "Hello, Alice! Today is Monday.");
}

void test_missing_variable() {
    PromptTemplate tpl("Hello, {{name}}!");
    Variables vars;

    bool threw = false;
    try {
        tpl.render(vars);
    } catch (const ConfigError& e) {
        threw = true;
    }
    assert(threw);
}

void test_no_variables() {
    PromptTemplate tpl("Just static text");
    Variables vars;

    std::string result = tpl.render(vars);
    assert(result == "Just static text");
}

void test_replacement_contains_braces() {
    // Test that replacement text containing {{ doesn't get re-processed
    PromptTemplate tpl("Template: {{var1}} and {{var2}}");
    Variables vars;
    vars["var1"] = "value with {{braces}}";
    vars["var2"] = "normal value";

    std::string result = tpl.render(vars);
    assert(result == "Template: value with {{braces}} and normal value");
}

int main() {
    test_simple_template();
    test_multiple_variables();
    test_missing_variable();
    test_no_variables();
    test_replacement_contains_braces();
    return 0;
}
