#ifndef AST_PRINTER_HPP
#define AST_PRINTER_HPP

#include "ast/ast.hpp"

#include <string>

// Pretty-prints an AST as an indented tree.
class AstPrinter {
public:
    std::string print(const Program& program);

private:
    int indentLevel = 0;

    std::string indent() const;
    std::string printStmt(const Stmt& stmt);
    std::string printExpr(const Expr& expr);
    std::string opToString(TokenType op) const;
};

#endif
