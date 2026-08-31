#ifndef COMPILER_HPP
#define COMPILER_HPP

#include "ast/ast.hpp"
#include "compiler/chunk.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

class CompileError : public std::runtime_error {
public:
    explicit CompileError(const std::string& message)
        : std::runtime_error(message) {}
};

// Compiles an AST Program into a top-level FunctionObject (script chunk).
class Compiler {
public:
    std::shared_ptr<FunctionObject> compile(const Program& program);

private:
    struct Local {
        std::string name;
        int depth;
    };

    FunctionObject* current = nullptr;
    int scopeDepth = 0;
    std::vector<Local> locals;

    Chunk& chunk();

    void emitByte(uint8_t byte, int line);
    void emitOp(OpCode op, int line);
    void emitOps(OpCode a, OpCode b, int line);
    void emitConstant(ConstantValue value, int line);
    uint8_t makeConstant(ConstantValue value);
    int emitJump(OpCode op, int line);
    void patchJump(int offset);
    void emitLoop(int loopStart, int line);
    void emitReturn(int line);

    void compileStmt(const Stmt& stmt);
    void compileExpr(const Expr& expr);

    void beginScope();
    void endScope(int line);
    int resolveLocal(const std::string& name) const;
    void addLocal(const std::string& name);

    void compileFunction(const FunctionDeclStmt& stmt);
};

#endif
