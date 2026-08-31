#include "compiler/compiler.hpp"

#include "lexer/token.hpp"

#include <stdexcept>

std::shared_ptr<FunctionObject> Compiler::compile(const Program& program) {
    auto script = std::make_shared<FunctionObject>();
    script->name = "<script>";
    script->arity = 0;

    current = script.get();
    scopeDepth = 0;
    locals.clear();

    for (const auto& stmt : program.statements) {
        compileStmt(*stmt);
    }

    // Implicit nil return at end of script
    emitOp(OP_NIL, 0);
    emitReturn(0);

    current = nullptr;
    return script;
}

Chunk& Compiler::chunk() {
    return current->chunk;
}

void Compiler::emitByte(uint8_t byte, int line) {
    chunk().writeByte(byte, line);
}

void Compiler::emitOp(OpCode op, int line) {
    chunk().writeOp(op, line);
}

void Compiler::emitOps(OpCode a, OpCode b, int line) {
    emitOp(a, line);
    emitOp(b, line);
}

uint8_t Compiler::makeConstant(ConstantValue value) {
    int index = chunk().addConstant(std::move(value));
    return static_cast<uint8_t>(index);
}

void Compiler::emitConstant(ConstantValue value, int line) {
    emitOp(OP_CONSTANT, line);
    emitByte(makeConstant(std::move(value)), line);
}

int Compiler::emitJump(OpCode op, int line) {
    return chunk().writeJump(op, line);
}

void Compiler::patchJump(int offset) {
    chunk().patchJump(offset);
}

void Compiler::emitLoop(int loopStart, int line) {
    chunk().writeLoop(loopStart, line);
}

void Compiler::emitReturn(int line) {
    emitOp(OP_RETURN, line);
}

void Compiler::beginScope() {
    scopeDepth++;
}

void Compiler::endScope(int line) {
    while (!locals.empty() && locals.back().depth >= scopeDepth) {
        emitOp(OP_POP, line);
        locals.pop_back();
    }
    scopeDepth--;
}

int Compiler::resolveLocal(const std::string& name) const {
    for (int i = static_cast<int>(locals.size()) - 1; i >= 0; --i) {
        if (locals[i].name == name) {
            return i;
        }
    }
    return -1;
}

void Compiler::addLocal(const std::string& name) {
    if (locals.size() >= 256) {
        throw CompileError("Too many local variables in function");
    }
    locals.push_back(Local{name, scopeDepth});
}

void Compiler::compileStmt(const Stmt& stmt) {
    if (auto* s = dynamic_cast<const ExprStmt*>(&stmt)) {
        compileExpr(*s->expression);
        emitOp(OP_POP, 0);
        return;
    }

    if (auto* s = dynamic_cast<const PrintStmt*>(&stmt)) {
        compileExpr(*s->expression);
        emitOp(OP_PRINT, 0);
        return;
    }

    if (auto* s = dynamic_cast<const VarDeclStmt*>(&stmt)) {
        compileExpr(*s->initializer);
        if (scopeDepth > 0) {
            addLocal(s->name);
            // value already on stack in the new local slot
        } else {
            emitOp(OP_DEFINE_GLOBAL, s->line);
            emitByte(makeConstant(ConstantValue::makeString(s->name)), s->line);
        }
        return;
    }

    if (auto* s = dynamic_cast<const BlockStmt*>(&stmt)) {
        beginScope();
        for (const auto& child : s->statements) {
            compileStmt(*child);
        }
        endScope(0);
        return;
    }

    if (auto* s = dynamic_cast<const IfStmt*>(&stmt)) {
        compileExpr(*s->condition);
        int thenJump = emitJump(OP_JUMP_IF_FALSE, 0);
        emitOp(OP_POP, 0);  // pop condition in then branch
        compileStmt(*s->thenBranch);

        int elseJump = emitJump(OP_JUMP, 0);
        patchJump(thenJump);
        emitOp(OP_POP, 0);  // pop condition in else/fallthrough

        if (s->elseBranch) {
            compileStmt(*s->elseBranch);
        }
        patchJump(elseJump);
        return;
    }

    if (auto* s = dynamic_cast<const WhileStmt*>(&stmt)) {
        int loopStart = static_cast<int>(chunk().code.size());
        compileExpr(*s->condition);
        int exitJump = emitJump(OP_JUMP_IF_FALSE, 0);
        emitOp(OP_POP, 0);
        compileStmt(*s->body);
        emitLoop(loopStart, 0);
        patchJump(exitJump);
        emitOp(OP_POP, 0);
        return;
    }

    if (auto* s = dynamic_cast<const FunctionDeclStmt*>(&stmt)) {
        compileFunction(*s);
        return;
    }

    if (auto* s = dynamic_cast<const ReturnStmt*>(&stmt)) {
        if (s->value) {
            compileExpr(*s->value);
        } else {
            emitOp(OP_NIL, s->line);
        }
        emitReturn(s->line);
        return;
    }

    throw CompileError("Unknown statement type in compiler");
}

void Compiler::compileFunction(const FunctionDeclStmt& stmt) {
    auto function = std::make_shared<FunctionObject>();
    function->name = stmt.name;
    function->arity = static_cast<int>(stmt.params.size());

    FunctionObject* enclosing = current;
    std::vector<Local> enclosingLocals = locals;
    int enclosingDepth = scopeDepth;

    current = function.get();
    locals.clear();
    scopeDepth = 0;
    beginScope();  // function body scope

    // Slot 0 reserved for the function itself (call frame); params follow.
    // For disassembly/VM compatibility with Crafting Interpreters layout:
    // locals[0] = function name (or ""), then params.
    addLocal(stmt.name);
    for (const auto& param : stmt.params) {
        addLocal(param);
    }

    for (const auto& bodyStmt : stmt.body) {
        compileStmt(*bodyStmt);
    }

    emitOp(OP_NIL, stmt.line);
    emitReturn(stmt.line);

    // Restore enclosing compiler state
    current = enclosing;
    locals = std::move(enclosingLocals);
    scopeDepth = enclosingDepth;

    // Define function as a global (or local if nested — Stage 4: always global at declare site depth)
    emitConstant(ConstantValue::makeFunction(function), stmt.line);
    if (scopeDepth > 0) {
        addLocal(stmt.name);
    } else {
        emitOp(OP_DEFINE_GLOBAL, stmt.line);
        emitByte(makeConstant(ConstantValue::makeString(stmt.name)), stmt.line);
    }
}

void Compiler::compileExpr(const Expr& expr) {
    if (auto* e = dynamic_cast<const LiteralExpr*>(&expr)) {
        switch (e->kind) {
            case LiteralExpr::Kind::Number:
                emitConstant(ConstantValue::makeNumber(e->numberValue), e->line);
                return;
            case LiteralExpr::Kind::String:
                emitConstant(ConstantValue::makeString(e->stringValue), e->line);
                return;
            case LiteralExpr::Kind::Boolean:
                emitOp(e->boolValue ? OP_TRUE : OP_FALSE, e->line);
                return;
        }
    }

    if (auto* e = dynamic_cast<const VariableExpr*>(&expr)) {
        int local = resolveLocal(e->name);
        if (local >= 0) {
            emitOp(OP_GET_LOCAL, e->line);
            emitByte(static_cast<uint8_t>(local), e->line);
        } else {
            emitOp(OP_GET_GLOBAL, e->line);
            emitByte(makeConstant(ConstantValue::makeString(e->name)), e->line);
        }
        return;
    }

    if (auto* e = dynamic_cast<const AssignExpr*>(&expr)) {
        compileExpr(*e->value);
        int local = resolveLocal(e->name);
        if (local >= 0) {
            emitOp(OP_SET_LOCAL, e->line);
            emitByte(static_cast<uint8_t>(local), e->line);
        } else {
            emitOp(OP_SET_GLOBAL, e->line);
            emitByte(makeConstant(ConstantValue::makeString(e->name)), e->line);
        }
        return;
    }

    if (auto* e = dynamic_cast<const UnaryExpr*>(&expr)) {
        compileExpr(*e->right);
        switch (e->op) {
            case TokenType::MINUS:
                emitOp(OP_NEGATE, e->line);
                break;
            case TokenType::BANG:
                emitOp(OP_NOT, e->line);
                break;
            default:
                throw CompileError("Unknown unary operator in compiler");
        }
        return;
    }

    if (auto* e = dynamic_cast<const BinaryExpr*>(&expr)) {
        // Short-circuit &&
        if (e->op == TokenType::AND) {
            compileExpr(*e->left);
            int endJump = emitJump(OP_JUMP_IF_FALSE, e->line);
            emitOp(OP_POP, e->line);
            compileExpr(*e->right);
            patchJump(endJump);
            return;
        }
        // Short-circuit ||
        if (e->op == TokenType::OR) {
            compileExpr(*e->left);
            int elseJump = emitJump(OP_JUMP_IF_FALSE, e->line);
            int endJump = emitJump(OP_JUMP, e->line);
            patchJump(elseJump);
            emitOp(OP_POP, e->line);
            compileExpr(*e->right);
            patchJump(endJump);
            return;
        }

        compileExpr(*e->left);
        compileExpr(*e->right);

        switch (e->op) {
            case TokenType::PLUS:
                emitOp(OP_ADD, e->line);
                break;
            case TokenType::MINUS:
                emitOp(OP_SUBTRACT, e->line);
                break;
            case TokenType::STAR:
                emitOp(OP_MULTIPLY, e->line);
                break;
            case TokenType::SLASH:
                emitOp(OP_DIVIDE, e->line);
                break;
            case TokenType::EQ_EQ:
                emitOp(OP_EQUAL, e->line);
                break;
            case TokenType::BANG_EQ:
                emitOp(OP_NOT_EQUAL, e->line);
                break;
            case TokenType::GREATER:
                emitOp(OP_GREATER, e->line);
                break;
            case TokenType::GREATER_EQ:
                emitOp(OP_GREATER_EQUAL, e->line);
                break;
            case TokenType::LESS:
                emitOp(OP_LESS, e->line);
                break;
            case TokenType::LESS_EQ:
                emitOp(OP_LESS_EQUAL, e->line);
                break;
            default:
                throw CompileError("Unknown binary operator in compiler");
        }
        return;
    }

    if (auto* e = dynamic_cast<const CallExpr*>(&expr)) {
        compileExpr(*e->callee);
        for (const auto& arg : e->arguments) {
            compileExpr(*arg);
        }
        emitOp(OP_CALL, e->line);
        emitByte(static_cast<uint8_t>(e->arguments.size()), e->line);
        return;
    }

    throw CompileError("Unknown expression type in compiler");
}
