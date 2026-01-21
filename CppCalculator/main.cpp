#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <stdexcept>
#include <cmath>

// 令牌类型
enum TokenType {
    NUMBER,
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    LPAREN,
    RPAREN,
    END
};

// 令牌结构体
struct Token {
    TokenType type;
    double value; // 仅当 type 为 NUMBER 时有效
};

class Calculator {
public:
    Calculator(const std::string& expression) : expr(expression), pos(0) {}

    double evaluate() {
        tokens = tokenize(expr);
        currentTokenIndex = 0;
        return parseExpression();
    }

private:
    std::string expr;
    size_t pos;
    std::vector<Token> tokens;
    size_t currentTokenIndex;

    // 词法分析：将字符串转换为令牌列表
    std::vector<Token> tokenize(const std::string& text) {
        std::vector<Token> result;
        for (size_t i = 0; i < text.length(); ++i) {
            char c = text[i];
            if (isspace(c)) continue;

            if (isdigit(c) || c == '.') {
                std::string numStr;
                while (i < text.length() && (isdigit(text[i]) || text[i] == '.')) {
                    numStr += text[i];
                    i++;
                }
                i--; // 回退一步，因为循环中多加了一次
                result.push_back({NUMBER, std::stod(numStr)});
            } else {
                switch (c) {
                    case '+': result.push_back({PLUS, 0}); break;
                    case '-': result.push_back({MINUS, 0}); break;
                    case '*': result.push_back({MULTIPLY, 0}); break;
                    case '/': result.push_back({DIVIDE, 0}); break;
                    case '(': result.push_back({LPAREN, 0}); break;
                    case ')': result.push_back({RPAREN, 0}); break;
                    default: 
                        throw std::runtime_error("未知字符: " + std::string(1, c));
                }
            }
        }
        result.push_back({END, 0});
        return result;
    }

    Token peek() {
        if (currentTokenIndex < tokens.size()) {
            return tokens[currentTokenIndex];
        }
        return {END, 0};
    }

    Token advance() {
        if (currentTokenIndex < tokens.size()) {
            return tokens[currentTokenIndex++];
        }
        return {END, 0};
    }

    // 解析表达式：处理加减法
    double parseExpression() {
        double left = parseTerm();
        while (peek().type == PLUS || peek().type == MINUS) {
            Token op = advance();
            double right = parseTerm();
            if (op.type == PLUS) {
                left += right;
            } else {
                left -= right;
            }
        }
        return left;
    }

    // 解析项：处理乘除法
    double parseTerm() {
        double left = parseFactor();
        while (peek().type == MULTIPLY || peek().type == DIVIDE) {
            Token op = advance();
            double right = parseFactor();
            if (op.type == MULTIPLY) {
                left *= right;
            } else {
                if (right == 0) throw std::runtime_error("除数不能为零");
                left /= right;
            }
        }
        return left;
    }

    // 解析因子：处理数字和括号
    double parseFactor() {
        Token token = advance();
        if (token.type == NUMBER) {
            return token.value;
        }
        if (token.type == LPAREN) {
            double result = parseExpression();
            if (advance().type != RPAREN) {
                throw std::runtime_error("缺少右括号");
            }
            return result;
        }
        if (token.type == MINUS) { // 处理负号
            return -parseFactor();
        }
        if (token.type == END) {
            throw std::runtime_error("表达式意外结束");
        }
        throw std::runtime_error("语法错误");
    }
};

int main(int argc, char* argv[]) {
    std::string expression;
    
    // 如果有命令行参数，则拼接参数作为表达式
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            expression += argv[i];
        }
    } else {
        std::cout << "请输入数学表达式 (例如: 3 + 4 * 2): ";
        std::getline(std::cin, expression);
    }

    if (expression.empty()) {
        return 0;
    }

    try {
        Calculator calc(expression);
        double result = calc.evaluate();
        std::cout << "结果: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
