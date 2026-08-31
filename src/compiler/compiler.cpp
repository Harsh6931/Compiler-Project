#include "compiler/compiler.hpp"

#include "lexer/token.hpp"

std::shared_ptr<FunctionObject> Compiler::compile(const Program& program) {
    auto script = std::make_shared<FunctionObject>();
    script->name = "<script>";
    script->arity = 0;

    CompilerFunction top;
    top.function = script.get();
    top.enclosing = nullptr;
    top.scopeDepth = 0;
    current = &top;

    for (const auto& stmt : program.statements) {
        compileStmt(*stmt);
    }

    emitOp(OP_NIL, 0);
    emitReturn(0);

    current = nullptr;
    return script;
}

Chunk& Compiler::chunk() {
    return current->function->chunk;
}

void Compiler::emitByte(uint8_t byte, int line) {
    chunk().writeByte(byte, line);
}

void Compiler::emitOp(OpCode op, int line) {
    chunk().writeOp(op, line);
}

uint8_t Compiler::makeConstant(ConstantValue value) {
    return static_cast<uint8_t>(chunk().addConstant(std::move(value)));
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
    current->scopeDepth++;
}

void Compiler::endScope(int line) {
    while (!current->locals.empty() &&
           current->locals.back().depth >= current->scopeDepth) {
        if (current->locals.back().isCaptured) {
            emitOp(OP_CLOSE_UPVALUE, line);
        } else {
            emitOp(OP_POP, line);
        }
        current->locals.pop_back();
    }
    current->scopeDepth--;
}

void Compiler::addLocal(const std::string& name) {
    if (current->locals.size() >= 256) {
        throw CompileError("Too many local variables in function");
    }
    current->locals.push_back(Local{name, current->scopeDepth, false});
}

int Compiler::resolveLocal(CompilerFunction* comp, const std::string& name) {
    for (int i = static_cast<int>(comp->locals.size()) - 1; i >= 0; --i) {
        if (comp->locals[i].name == name) {
            return i;
        }
    }
    return -1;
}

int Compiler::addUpvalue(CompilerFunction* comp, uint8_t index, bool isLocal) {
    for (size_t i = 0; i < comp->upvalues.size(); ++i) {
        if (comp->upvalues[i].index == index &&
            comp->upvalues[i].isLocal == isLocal) {
            return static_cast<int>(i);
        }
    }
    if (comp->upvalues.size() >= 256) {
        throw CompileError("Too many closure variables in function");
    }
    comp->upvalues.push_back(UpvalueDesc{isLocal, index});
    return static_cast<int>(comp->upvalues.size() - 1);
}

int Compiler::resolveUpvalue(CompilerFunction* comp, const std::string& name) {
    if (comp->enclosing == nullptr) {
        return -1;
    }

    int local = resolveLocal(comp->enclosing, name);
    if (local != -1) {
        comp->enclosing->locals[local].isCaptured = true;
        return addUpvalue(comp, static_cast<uint8_t>(local), true);
    }

    int upvalue = resolveUpvalue(comp->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(comp, static_cast<uint8_t>(upvalue), false);
    }

    return -1;
}

void Compiler::namedVariable(const std::string& name, int line, bool assign) {
    OpCode getOp;
    OpCode setOp;
    int arg = resolveLocal(current, name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } else {
        arg = makeConstant(ConstantValue::makeString(name));
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    emitOp(assign ? setOp : getOp, line);
    emitByte(static_cast<uint8_t>(arg), line);
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
        if (current->scopeDepth > 0) {
            addLocal(s->name);
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
        emitOp(OP_POP, 0);
        compileStmt(*s->thenBranch);
        int elseJump = emitJump(OP_JUMP, 0);
        patchJump(thenJump);
        emitOp(OP_POP, 0);
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

    CompilerFunction nested;
    nested.function = function.get();
    nested.enclosing = current;
    nested.scopeDepth = 0;

    CompilerFunction* enclosing = current;
    current = &nested;

    beginScope();
    addLocal(stmt.name);
    for (const auto& param : stmt.params) {
        addLocal(param);
    }

    for (const auto& bodyStmt : stmt.body) {
        compileStmt(*bodyStmt);
    }

    emitOp(OP_NIL, stmt.line);
    emitReturn(stmt.line);

    function->upvalues = nested.upvalues;
    current = enclosing;

    emitOp(OP_CLOSURE, stmt.line);
    emitByte(makeConstant(ConstantValue::makeFunction(function)), stmt.line);
    for (const auto& uv : function->upvalues) {
        emitByte(uv.isLocal ? 1 : 0, stmt.line);
        emitByte(uv.index, stmt.line);
    }

    if (current->scopeDepth > 0) {
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
        namedVariable(e->name, e->line, false);
        return;
    }

    if (auto* e = dynamic_cast<const AssignExpr*>(&expr)) {
        compileExpr(*e->value);
        namedVariable(e->name, e->line, true);
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
        if (e->op == TokenType::AND) {
            compileExpr(*e->left);
            int endJump = emitJump(OP_JUMP_IF_FALSE, e->line);
            emitOp(OP_POP, e->line);
            compileExpr(*e->right);
            patchJump(endJump);
            return;
        }
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

    // Array / index nodes added in Stage 6 slice 4 — handled below when present
    if (auto* e = dynamic_cast<const ArrayExpr*>(&expr)) {
        for (const auto& el : e->elements) {
            compileExpr(*el);
        }
        emitOp(OP_BUILD_ARRAY, e->line);
        emitByte(static_cast<uint8_t>(e->elements.size()), e->line);
        return;
    }

    if (auto* e = dynamic_cast<const IndexExpr*>(&expr)) {
        compileExpr(*e->object);
        compileExpr(*e->index);
        emitOp(OP_INDEX_GET, e->line);
        return;
    }

    if (auto* e = dynamic_cast<const IndexAssignExpr*>(&expr)) {
        compileExpr(*e->object);
        compileExpr(*e->index);
        compileExpr(*e->value);
        emitOp(OP_INDEX_SET, e->line);
        return;
    }

    throw CompileError("Unknown expression type in compiler");
}
