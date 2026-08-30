#include "interpreter/interpreter.hpp"

#include "lexer/token.hpp"

#include <iostream>
#include <utility>

Interpreter::Interpreter()
    : globals(std::make_shared<Environment>()), environment(globals) {}

void Interpreter::interpret(const Program& program) {
    try {
        for (const auto& stmt : program.statements) {
            execute(*stmt);
        }
    } catch (const ReturnSignal&) {
        throw RuntimeError(0, "Cannot return from top-level code");
    }
}

void Interpreter::execute(const Stmt& stmt) {
    if (auto* s = dynamic_cast<const ExprStmt*>(&stmt)) {
        evaluate(*s->expression);
        return;
    }

    if (auto* s = dynamic_cast<const PrintStmt*>(&stmt)) {
        Value value = evaluate(*s->expression);
        std::cout << value.toString() << "\n";
        return;
    }

    if (auto* s = dynamic_cast<const VarDeclStmt*>(&stmt)) {
        Value value = evaluate(*s->initializer);
        environment->define(s->name, value);
        return;
    }

    if (auto* s = dynamic_cast<const BlockStmt*>(&stmt)) {
        executeBlock(s->statements, std::make_shared<Environment>(environment));
        return;
    }

    if (auto* s = dynamic_cast<const IfStmt*>(&stmt)) {
        if (evaluate(*s->condition).isTruthy()) {
            execute(*s->thenBranch);
        } else if (s->elseBranch) {
            execute(*s->elseBranch);
        }
        return;
    }

    if (auto* s = dynamic_cast<const WhileStmt*>(&stmt)) {
        while (evaluate(*s->condition).isTruthy()) {
            execute(*s->body);
        }
        return;
    }

    if (auto* s = dynamic_cast<const FunctionDeclStmt*>(&stmt)) {
        // Bind the function in the current environment; closure captures it.
        Value fn = Value::makeFunction(s, environment);
        environment->define(s->name, fn);
        return;
    }

    if (auto* s = dynamic_cast<const ReturnStmt*>(&stmt)) {
        Value value = Value::makeNil();
        if (s->value) {
            value = evaluate(*s->value);
        }
        throw ReturnSignal(std::move(value));
    }
}

void Interpreter::executeBlock(const std::vector<StmtPtr>& statements,
                               std::shared_ptr<Environment> env) {
    std::shared_ptr<Environment> previous = environment;
    environment = std::move(env);
    try {
        for (const auto& statement : statements) {
            execute(*statement);
        }
    } catch (...) {
        environment = previous;
        throw;
    }
    environment = previous;
}

Value Interpreter::evaluate(const Expr& expr) {
    if (auto* e = dynamic_cast<const LiteralExpr*>(&expr)) {
        switch (e->kind) {
            case LiteralExpr::Kind::Number:
                return Value::makeNumber(e->numberValue);
            case LiteralExpr::Kind::String:
                return Value::makeString(e->stringValue);
            case LiteralExpr::Kind::Boolean:
                return Value::makeBoolean(e->boolValue);
        }
    }

    if (auto* e = dynamic_cast<const VariableExpr*>(&expr)) {
        try {
            return environment->get(e->name);
        } catch (const std::runtime_error&) {
            throw RuntimeError(e->line, "Undefined variable '" + e->name + "'");
        }
    }

    if (auto* e = dynamic_cast<const AssignExpr*>(&expr)) {
        Value value = evaluate(*e->value);
        try {
            environment->assign(e->name, value);
        } catch (const std::runtime_error&) {
            throw RuntimeError(e->line, "Undefined variable '" + e->name + "'");
        }
        return value;
    }

    if (auto* e = dynamic_cast<const UnaryExpr*>(&expr)) {
        Value right = evaluate(*e->right);
        return evaluateUnary(e->op, right, e->line);
    }

    if (auto* e = dynamic_cast<const BinaryExpr*>(&expr)) {
        // Short-circuit for && and ||
        if (e->op == TokenType::AND) {
            Value left = evaluate(*e->left);
            if (!left.isTruthy()) {
                return left;
            }
            return evaluate(*e->right);
        }
        if (e->op == TokenType::OR) {
            Value left = evaluate(*e->left);
            if (left.isTruthy()) {
                return left;
            }
            return evaluate(*e->right);
        }

        Value left = evaluate(*e->left);
        Value right = evaluate(*e->right);
        return evaluateBinary(e->op, left, right, e->line);
    }

    if (auto* e = dynamic_cast<const CallExpr*>(&expr)) {
        Value callee = evaluate(*e->callee);

        std::vector<Value> arguments;
        arguments.reserve(e->arguments.size());
        for (const auto& arg : e->arguments) {
            arguments.push_back(evaluate(*arg));
        }

        return callFunction(callee, arguments, e->line);
    }

    throw RuntimeError(0, "Unknown expression type");
}

Value Interpreter::evaluateUnary(TokenType op, const Value& right, int line) {
    switch (op) {
        case TokenType::BANG:
            return Value::makeBoolean(!right.isTruthy());
        case TokenType::MINUS:
            checkNumberOperand(op, right, line);
            return Value::makeNumber(-right.number);
        default:
            throw RuntimeError(line, "Unknown unary operator");
    }
}

Value Interpreter::evaluateBinary(TokenType op,
                                  const Value& left,
                                  const Value& right,
                                  int line) {
    switch (op) {
        case TokenType::PLUS:
            if (left.type == Value::Type::Number && right.type == Value::Type::Number) {
                return Value::makeNumber(left.number + right.number);
            }
            if (left.type == Value::Type::String && right.type == Value::Type::String) {
                return Value::makeString(left.string + right.string);
            }
            throw RuntimeError(line, "Operands of '+' must be two numbers or two strings");

        case TokenType::MINUS:
            checkNumberOperands(op, left, right, line);
            return Value::makeNumber(left.number - right.number);

        case TokenType::STAR:
            checkNumberOperands(op, left, right, line);
            return Value::makeNumber(left.number * right.number);

        case TokenType::SLASH:
            checkNumberOperands(op, left, right, line);
            if (right.number == 0) {
                throw RuntimeError(line, "Division by zero");
            }
            return Value::makeNumber(left.number / right.number);

        case TokenType::GREATER:
            checkNumberOperands(op, left, right, line);
            return Value::makeBoolean(left.number > right.number);

        case TokenType::GREATER_EQ:
            checkNumberOperands(op, left, right, line);
            return Value::makeBoolean(left.number >= right.number);

        case TokenType::LESS:
            checkNumberOperands(op, left, right, line);
            return Value::makeBoolean(left.number < right.number);

        case TokenType::LESS_EQ:
            checkNumberOperands(op, left, right, line);
            return Value::makeBoolean(left.number <= right.number);

        case TokenType::EQ_EQ:
            return Value::makeBoolean(isEqual(left, right));

        case TokenType::BANG_EQ:
            return Value::makeBoolean(!isEqual(left, right));

        default:
            throw RuntimeError(line, "Unknown binary operator");
    }
}

Value Interpreter::callFunction(const Value& callee,
                                const std::vector<Value>& args,
                                int line) {
    if (callee.type != Value::Type::Function) {
        throw RuntimeError(line, "Can only call functions");
    }

    const FunctionDeclStmt* fn = callee.declaration;
    if (args.size() != fn->params.size()) {
        throw RuntimeError(
            line,
            "Expected " + std::to_string(fn->params.size()) + " arguments but got " +
                std::to_string(args.size()));
    }

    auto callEnv = std::make_shared<Environment>(callee.closure);
    for (size_t i = 0; i < fn->params.size(); ++i) {
        callEnv->define(fn->params[i], args[i]);
    }

    try {
        executeBlock(fn->body, callEnv);
    } catch (const ReturnSignal& ret) {
        return ret.value;
    }

    return Value::makeNil();
}

bool Interpreter::isEqual(const Value& a, const Value& b) const {
    if (a.type != b.type) {
        return false;
    }
    switch (a.type) {
        case Value::Type::Nil:
            return true;
        case Value::Type::Number:
            return a.number == b.number;
        case Value::Type::Boolean:
            return a.boolean == b.boolean;
        case Value::Type::String:
            return a.string == b.string;
        case Value::Type::Function:
            return a.declaration == b.declaration;
    }
    return false;
}

void Interpreter::checkNumberOperand(TokenType /*op*/, const Value& operand, int line) {
    if (operand.type != Value::Type::Number) {
        throw RuntimeError(line, "Operand must be a number");
    }
}

void Interpreter::checkNumberOperands(TokenType /*op*/,
                                      const Value& left,
                                      const Value& right,
                                      int line) {
    if (left.type != Value::Type::Number || right.type != Value::Type::Number) {
        throw RuntimeError(line, "Operands must be numbers");
    }
}
