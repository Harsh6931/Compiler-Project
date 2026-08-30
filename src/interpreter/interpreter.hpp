#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include "ast/ast.hpp"
#include "interpreter/environment.hpp"
#include "interpreter/value.hpp"

#include <memory>
#include <stdexcept>
#include <string>

// Thrown by return statements to unwind out of a function call.
class ReturnSignal : public std::runtime_error {
public:
    Value value;

    explicit ReturnSignal(Value value)
        : std::runtime_error("return"), value(std::move(value)) {}
};

class RuntimeError : public std::runtime_error {
public:
    int line;

    RuntimeError(int line, const std::string& message)
        : std::runtime_error("Runtime error at line " + std::to_string(line) +
                             ": " + message),
          line(line) {}
};

// main walk
// Tree-walking interpreter: walks the AST and executes it directly.
class Interpreter {
public:
    Interpreter();

    void interpret(const Program& program);

private:
    std::shared_ptr<Environment> globals;
    std::shared_ptr<Environment> environment;

    void execute(const Stmt& stmt);
    void executeBlock(const std::vector<StmtPtr>& statements,
                      std::shared_ptr<Environment> env);

    Value evaluate(const Expr& expr);

    Value evaluateBinary(TokenType op, const Value& left, const Value& right, int line);
    Value evaluateUnary(TokenType op, const Value& right, int line);
    Value callFunction(const Value& callee, const std::vector<Value>& args, int line);

    bool isEqual(const Value& a, const Value& b) const;
    void checkNumberOperand(TokenType op, const Value& operand, int line);
    void checkNumberOperands(TokenType op, const Value& left, const Value& right, int line);
};

#endif
