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
        int depth = 0;
        bool isCaptured = false;
    };

    struct CompilerFunction {
        FunctionObject* function = nullptr;
        std::vector<Local> locals;
        std::vector<UpvalueDesc> upvalues;
        int scopeDepth = 0;
        CompilerFunction* enclosing = nullptr;
    };

    CompilerFunction* current = nullptr;

    Chunk& chunk();
    void emitByte(uint8_t byte, int line);
    void emitOp(OpCode op, int line);
    void emitConstant(ConstantValue value, int line);
    uint8_t makeConstant(ConstantValue value);
    int emitJump(OpCode op, int line);
    void patchJump(int offset);
    void emitLoop(int loopStart, int line);
    void emitReturn(int line);

    void beginScope();
    void endScope(int line);
    void addLocal(const std::string& name);
    int resolveLocal(CompilerFunction* comp, const std::string& name);
    int addUpvalue(CompilerFunction* comp, uint8_t index, bool isLocal);
    int resolveUpvalue(CompilerFunction* comp, const std::string& name);
    void namedVariable(const std::string& name, int line, bool assign);

    // Compile a statement list; skip code after an unconditional return (DCE).
    // Returns true if the list ends with a compiled return.
    bool compileStatements(const std::vector<StmtPtr>& statements);

    void compileStmt(const Stmt& stmt);
    void compileExpr(const Expr& expr);
    void compileFunction(const FunctionDeclStmt& stmt);
};

#endif
