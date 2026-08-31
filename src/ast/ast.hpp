#ifndef AST_HPP
#define AST_HPP

#include "lexer/token.hpp"

#include <memory>
#include <string>
#include <vector>

// defining all AST nodes types and their relationships 

// Forward declarations
struct Expr;   // expr is a base class for all expressions, all will inherit from this
struct Stmt;   // stmt is a base class for all statements, all will inherit from this

using ExprPtr = std::unique_ptr<Expr>; // pointer 
using StmtPtr = std::unique_ptr<Stmt>;

struct Expr {
    virtual ~Expr() = default;  // destructor
};

// NUMBER, STRING, true, false
struct LiteralExpr : Expr {
    enum class Kind { Number, String, Boolean };

    Kind kind;
    std::string stringValue;  // for strings (and display)
    long long numberValue = 0;
    bool boolValue = false;
    int line;

    static ExprPtr number(long long value, int line) {  // stores type identifier-> data value-> string copy of value(for test and debugging)-> source line number-> create unique pointer to LiteralExpr object
        auto e = std::make_unique<LiteralExpr>();
        e->kind = Kind::Number;
        e->numberValue = value;
        e->stringValue = std::to_string(value);
        e->line = line;
        return e;
    }

    static ExprPtr string(std::string value, int line) {
        auto e = std::make_unique<LiteralExpr>();
        e->kind = Kind::String;
        e->stringValue = std::move(value);
        e->line = line;
        return e;
    }

    static ExprPtr boolean(bool value, int line) {
        auto e = std::make_unique<LiteralExpr>();
        e->kind = Kind::Boolean;
        e->boolValue = value;
        e->stringValue = value ? "true" : "false";
        e->line = line;
        return e;
    }
};

struct VariableExpr : Expr {
    std::string name;
    int line;

    VariableExpr(std::string name, int line)
        : name(std::move(name)), line(line) {}
};

struct UnaryExpr : Expr {  // -2
    TokenType op;
    ExprPtr right;
    int line;

    UnaryExpr(TokenType op, ExprPtr right, int line)
        : op(op), right(std::move(right)), line(line) {}
};

struct BinaryExpr : Expr {  // (2+3,2-3)
    ExprPtr left; 
    TokenType op;
    ExprPtr right;
    int line;

    BinaryExpr(ExprPtr left, TokenType op, ExprPtr right, int line)
        : left(std::move(left)), op(op), right(std::move(right)), line(line) {}
};

struct AssignExpr : Expr {  // a=2
    std::string name;
    ExprPtr value;
    int line;

    AssignExpr(std::string name, ExprPtr value, int line)
        : name(std::move(name)), value(std::move(value)), line(line) {}
};

struct CallExpr : Expr { // function call node in AST (i.e. print(2+3))
    ExprPtr callee;  //FUNCTION NAME 
    std::vector<ExprPtr> arguments; //ARGUMENTS PASSED TO FUNCTION
    int line;  //SOURCE LINE NUMBER

    CallExpr(ExprPtr callee, std::vector<ExprPtr> arguments, int line)
        : callee(std::move(callee)), arguments(std::move(arguments)), line(line) {}
};

struct ArrayExpr : Expr {
    std::vector<ExprPtr> elements;
    int line;

    ArrayExpr(std::vector<ExprPtr> elements, int line)
        : elements(std::move(elements)), line(line) {}
};

struct IndexExpr : Expr {
    ExprPtr object;
    ExprPtr index;
    int line;

    IndexExpr(ExprPtr object, ExprPtr index, int line)
        : object(std::move(object)), index(std::move(index)), line(line) {}
};

struct IndexAssignExpr : Expr {
    ExprPtr object;
    ExprPtr index;
    ExprPtr value;
    int line;

    IndexAssignExpr(ExprPtr object, ExprPtr index, ExprPtr value, int line)
        : object(std::move(object)),
          index(std::move(index)),
          value(std::move(value)),
          line(line) {}
};

// Statements are the building blocks of the program.
// They are the instructions that the compiler or interpreter will execute.

// --- Statements ---

struct Stmt {
    virtual ~Stmt() = default;
};

struct ExprStmt : Stmt {
    ExprPtr expression;  //EXPRESSION TO BE EVALUATED

    explicit ExprStmt(ExprPtr expression)
        : expression(std::move(expression)) {}
};

struct PrintStmt : Stmt {
    ExprPtr expression;  //EXPRESSION TO BE PRINTED

    explicit PrintStmt(ExprPtr expression)
        : expression(std::move(expression)) {}
};

struct VarDeclStmt : Stmt {  // X= 10
    std::string name;    
    ExprPtr initializer;
    int line;

    VarDeclStmt(std::string name, ExprPtr initializer, int line)
        : name(std::move(name)), initializer(std::move(initializer)), line(line) {}
};

struct BlockStmt : Stmt {  //{ let x = 1; print x; 
    std::vector<StmtPtr> statements;

    explicit BlockStmt(std::vector<StmtPtr> statements)
        : statements(std::move(statements)) {}
};

struct IfStmt : Stmt {
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch;  // may be null

    IfStmt(ExprPtr condition, StmtPtr thenBranch, StmtPtr elseBranch)
        : condition(std::move(condition)),
          thenBranch(std::move(thenBranch)),
          elseBranch(std::move(elseBranch)) {}
};

struct WhileStmt : Stmt {
    ExprPtr condition;
    StmtPtr body;

    WhileStmt(ExprPtr condition, StmtPtr body)
        : condition(std::move(condition)), body(std::move(body)) {}
};

struct FunctionDeclStmt : Stmt {
    std::string name;
    std::vector<std::string> params;
    std::vector<StmtPtr> body;
    int line;

    FunctionDeclStmt(std::string name,
                     std::vector<std::string> params,
                     std::vector<StmtPtr> body,
                     int line)
        : name(std::move(name)),
          params(std::move(params)),
          body(std::move(body)),
          line(line) {}
};

struct ReturnStmt : Stmt {
    ExprPtr value;  // may be null
    int line;

    ReturnStmt(ExprPtr value, int line)
        : value(std::move(value)), line(line) {}
};

// Top-level program = list of statements
struct Program {
    std::vector<StmtPtr> statements;
};

#endif
