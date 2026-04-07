#include <boostchain/tool.hpp>
#include <boostchain/error.hpp>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <cmath>
#include <iomanip>


namespace boostchain {

// ============================================================================
// CalculatorTool
// ============================================================================

ToolDefinition CalculatorTool::get_definition() const {
    ToolDefinition def;
    def.name = "calculator";
    def.description = "Evaluate a mathematical expression and return the result. "
                      "Supports basic arithmetic: addition (+), subtraction (-), "
                      "multiplication (*), division (/), and parentheses.";

    ToolParameter expr_param;
    expr_param.type = "string";
    expr_param.description = "The mathematical expression to evaluate (e.g., '2 + 3 * 4')";
    expr_param.required = true;
    def.parameters["expression"] = expr_param;

    return def;
}

namespace {

// Simple recursive-descent expression parser for basic arithmetic
struct Parser {
    const std::string& expr;
    size_t pos = 0;

    explicit Parser(const std::string& e) : expr(e) {}

    void skip_spaces() {
        while (pos < expr.size() && std::isspace(expr[pos])) pos++;
    }

    char peek() {
        skip_spaces();
        if (pos < expr.size()) return expr[pos];
        return '\0';
    }

    char consume() {
        skip_spaces();
        if (pos < expr.size()) return expr[pos++];
        return '\0';
    }

    bool match(char c) {
        if (peek() == c) {
            pos++;
            return true;
        }
        return false;
    }

    double parse_number() {
        skip_spaces();
        size_t start = pos;
        // Handle optional negative sign
        if (pos < expr.size() && expr[pos] == '-') pos++;
        // Integer part
        while (pos < expr.size() && std::isdigit(expr[pos])) pos++;
        // Decimal part
        if (pos < expr.size() && expr[pos] == '.') {
            pos++;
            while (pos < expr.size() && std::isdigit(expr[pos])) pos++;
        }
        std::string num_str = expr.substr(start, pos - start);
        return std::stod(num_str);
    }

    double parse_factor() {
        skip_spaces();
        if (peek() == '(') {
            consume(); // '('
            double val = parse_expression();
            consume(); // ')'
            return val;
        }
        return parse_number();
    }

    double parse_term() {
        double left = parse_factor();
        while (true) {
            if (peek() == '*') {
                consume();
                left *= parse_factor();
            } else if (peek() == '/') {
                consume();
                double right = parse_factor();
                if (right == 0.0) throw ToolError("calculator", "division by zero");
                left /= right;
            } else {
                break;
            }
        }
        return left;
    }

    double parse_expression() {
        double left = parse_term();
        while (true) {
            if (peek() == '+') {
                consume();
                left += parse_term();
            } else if (peek() == '-') {
                consume();
                left -= parse_term();
            } else {
                break;
            }
        }
        return left;
    }
};

// Minimal JSON string value extraction: expects {"key": "value"} or {"key": "value", ...}
std::string extract_json_string_value(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";

    // Find colon after key
    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return "";
    pos++;

    // Skip whitespace
    while (pos < json.size() && std::isspace(json[pos])) pos++;
    if (pos >= json.size() || json[pos] != '"') return "";
    pos++; // skip opening quote

    // Find closing quote (handle escaped quotes)
    std::string result;
    while (pos < json.size()) {
        if (json[pos] == '\\' && pos + 1 < json.size()) {
            result += json[pos + 1];
            pos += 2;
        } else if (json[pos] == '"') {
            break;
        } else {
            result += json[pos];
            pos++;
        }
    }
    return result;
}

} // anonymous namespace

ToolResult CalculatorTool::execute(const std::string& arguments_json) {
    ToolResult result;

    try {
        std::string expression = extract_json_string_value(arguments_json, "expression");
        if (expression.empty()) {
            result.success = false;
            result.error = "Missing required parameter: expression";
            return result;
        }

        Parser parser(expression);
        double value = parser.parse_expression();

        // Format result: remove trailing zeros
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(10);
        oss << value;
        std::string formatted = oss.str();

        // Remove trailing zeros after decimal point
        size_t dot = formatted.find('.');
        if (dot != std::string::npos) {
            size_t last_nonzero = formatted.find_last_not_of('0');
            if (last_nonzero != std::string::npos) {
                if (formatted[last_nonzero] == '.') {
                    formatted = formatted.substr(0, last_nonzero);
                } else {
                    formatted = formatted.substr(0, last_nonzero + 1);
                }
            }
        }

        result.content = formatted;
        result.success = true;
    } catch (const ToolError& e) {
        result.success = false;
        result.error = e.what();
    } catch (const std::exception& e) {
        result.success = false;
        result.error = std::string("Parse error: ") + e.what();
    }

    return result;
}

} // namespace boostchain
