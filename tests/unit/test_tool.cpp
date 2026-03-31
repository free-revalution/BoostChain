#include <boostchain/tool.hpp>
#include <cassert>
#include <string>
#include <map>

using namespace boostchain;

void test_tool_result_struct() {
    ToolResult result;
    result.content = "42";
    result.success = true;

    assert(result.content == "42");
    assert(result.success == true);
    assert(result.error.empty());
}

void test_tool_result_error() {
    ToolResult result;
    result.success = false;
    result.error = "something went wrong";

    assert(result.success == false);
    assert(result.error == "something went wrong");
    assert(result.content.empty());
}

void test_calculator_definition() {
    CalculatorTool calc;
    ToolDefinition def = calc.get_definition();

    assert(def.name == "calculator");
    assert(!def.description.empty());
    assert(def.parameters.count("expression") == 1);

    auto& param = def.parameters.at("expression");
    assert(param.type == "string");
    assert(param.required == true);
}

void test_calculator_get_name() {
    CalculatorTool calc;
    assert(calc.get_name() == "calculator");
}

void test_calculator_addition() {
    CalculatorTool calc;
    auto result = calc.execute("{\"expression\": \"2 + 3\"}");
    assert(result.success == true);
    assert(result.content == "5");
}

void test_calculator_subtraction() {
    CalculatorTool calc;
    auto result = calc.execute("{\"expression\": \"10 - 4\"}");
    assert(result.success == true);
    assert(result.content == "6");
}

void test_calculator_multiplication() {
    CalculatorTool calc;
    auto result = calc.execute("{\"expression\": \"3 * 7\"}");
    assert(result.success == true);
    assert(result.content == "21");
}

void test_calculator_division() {
    CalculatorTool calc;
    auto result = calc.execute("{\"expression\": \"15 / 4\"}");
    assert(result.success == true);
    // 15/4 = 3.75
    assert(result.content == "3.75");
}

void test_calculator_precedence() {
    CalculatorTool calc;
    auto result = calc.execute("{\"expression\": \"2 + 3 * 4\"}");
    assert(result.success == true);
    assert(result.content == "14"); // 3*4=12, 12+2=14
}

void test_calculator_parentheses() {
    CalculatorTool calc;
    auto result = calc.execute("{\"expression\": \"(2 + 3) * 4\"}");
    assert(result.success == true);
    assert(result.content == "20");
}

void test_calculator_complex() {
    CalculatorTool calc;
    auto result = calc.execute("{\"expression\": \"(10 + 5) * (3 - 1) / 5\"}");
    assert(result.success == true);
    assert(result.content == "6"); // 15 * 2 / 5 = 6
}

void test_calculator_division_by_zero() {
    CalculatorTool calc;
    auto result = calc.execute("{\"expression\": \"10 / 0\"}");
    assert(result.success == false);
    assert(!result.error.empty());
}

void test_calculator_missing_param() {
    CalculatorTool calc;
    auto result = calc.execute("{\"wrong_key\": \"2 + 2\"}");
    assert(result.success == false);
    assert(result.error.find("expression") != std::string::npos);
}

void test_calculator_empty_json() {
    CalculatorTool calc;
    auto result = calc.execute("{}");
    assert(result.success == false);
}

void test_calculator_negative_numbers() {
    CalculatorTool calc;
    auto result = calc.execute("{\"expression\": \"5 + -3\"}");
    assert(result.success == true);
    assert(result.content == "2");
}

void test_calculator_decimal_result() {
    CalculatorTool calc;
    auto result = calc.execute("{\"expression\": \"1 / 3\"}");
    assert(result.success == true);
    // Should contain decimal point
    assert(result.content.find('.') != std::string::npos);
}

void test_itool_polymorphism() {
    std::unique_ptr<ITool> tool = std::make_unique<CalculatorTool>();
    assert(tool->get_name() == "calculator");

    ToolDefinition def = tool->get_definition();
    assert(def.name == "calculator");

    auto result = tool->execute("{\"expression\": \"1 + 1\"}");
    assert(result.success == true);
    assert(result.content == "2");
}

void test_tool_definition_struct() {
    ToolDefinition def;
    def.name = "test_tool";
    def.description = "A test tool";

    ToolParameter param;
    param.type = "string";
    param.description = "A string parameter";
    param.required = true;
    def.parameters["str_param"] = param;

    assert(def.name == "test_tool");
    assert(def.parameters.size() == 1);
    assert(def.parameters.at("str_param").required == true);
}

void test_tool_parameter_optional() {
    ToolParameter param;
    param.type = "number";
    param.description = "optional number";
    param.required = false;

    assert(param.required == false);
    assert(!param.enum_values.has_value());
}

void test_tool_parameter_with_enum() {
    ToolParameter param;
    param.type = "string";
    param.description = "unit type";
    param.required = true;
    param.enum_values = "[\"celsius\", \"fahrenheit\"]";

    assert(param.enum_values.has_value());
    assert(param.enum_values->find("celsius") != std::string::npos);
}

int main() {
    test_tool_result_struct();
    test_tool_result_error();
    test_calculator_definition();
    test_calculator_get_name();
    test_calculator_addition();
    test_calculator_subtraction();
    test_calculator_multiplication();
    test_calculator_division();
    test_calculator_precedence();
    test_calculator_parentheses();
    test_calculator_complex();
    test_calculator_division_by_zero();
    test_calculator_missing_param();
    test_calculator_empty_json();
    test_calculator_negative_numbers();
    test_calculator_decimal_result();
    test_itool_polymorphism();
    test_tool_definition_struct();
    test_tool_parameter_optional();
    test_tool_parameter_with_enum();
    return 0;
}
