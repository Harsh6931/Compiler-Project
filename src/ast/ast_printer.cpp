#include "ast/ast_printer.hpp"

#include <sstream>

std::string AstPrinter::print(const Program& program) {
    std::ostringstream out;
    out << "Program\n";
    indentLevel = 1;
    for (const auto& stmt : program.statements) {
        out << printStmt(*stmt);
    }
    return out.str();
}

std::string AstPrinter::indent() const {
    return std::string(indentLevel * 2, ' ');
}

std::string AstPrinter::opToString(TokenType op) const {
    switch (op) {
        case TokenType::PLUS: return "+";
        case TokenType::MINUS: return "-";
        case TokenType::STAR: return "*";
        case TokenType::SLASH: return "/";
        case TokenType::BANG: return "!";
        case TokenType::EQ_EQ: return "==";
        case TokenType::BANG_EQ: return "!=";
        case TokenType::LESS: return "<";
        case TokenType::GREATER: return ">";
        case TokenType::LESS_EQ: return "<=";
        case TokenType::GREATER_EQ: return ">=";
        case TokenType::AND: return "&&";
        case TokenType::OR: return "||";
        case TokenType::EQ: return "=";
        default: return tokenTypeToString(op);
    }
}

std::string AstPrinter::printStmt(const Stmt& stmt) {
    std::ostringstream out;

    if (auto* s = dynamic_cast<const VarDeclStmt*>(&stmt)) {
        out << indent() << "VarDeclStmt " << s->name << "\n";
        indentLevel++;
        out << printExpr(*s->initializer);
        indentLevel--;
    } else if (auto* s = dynamic_cast<const PrintStmt*>(&stmt)) {
        out << indent() << "PrintStmt\n";
        indentLevel++;
        out << printExpr(*s->expression);
        indentLevel--;
    } else if (auto* s = dynamic_cast<const ExprStmt*>(&stmt)) {
        out << indent() << "ExprStmt\n";
        indentLevel++;
        out << printExpr(*s->expression);
        indentLevel--;
    } else if (auto* s = dynamic_cast<const BlockStmt*>(&stmt)) {
        out << indent() << "BlockStmt\n";
        indentLevel++;
        for (const auto& child : s->statements) {
            out << printStmt(*child);
        }
        indentLevel--;
    } else if (auto* s = dynamic_cast<const IfStmt*>(&stmt)) {
        out << indent() << "IfStmt\n";
        indentLevel++;
        out << indent() << "condition:\n";
        indentLevel++;
        out << printExpr(*s->condition);
        indentLevel--;
        out << indent() << "then:\n";
        indentLevel++;
        out << printStmt(*s->thenBranch);
        indentLevel--;
        if (s->elseBranch) {
            out << indent() << "else:\n";
            indentLevel++;
            out << printStmt(*s->elseBranch);
            indentLevel--;
        }
        indentLevel--;
    } else if (auto* s = dynamic_cast<const WhileStmt*>(&stmt)) {
        out << indent() << "WhileStmt\n";
        indentLevel++;
        out << indent() << "condition:\n";
        indentLevel++;
        out << printExpr(*s->condition);
        indentLevel--;
        out << indent() << "body:\n";
        indentLevel++;
        out << printStmt(*s->body);
        indentLevel--;
        indentLevel--;
    } else if (auto* s = dynamic_cast<const FunctionDeclStmt*>(&stmt)) {
        out << indent() << "FunctionDeclStmt " << s->name << "(";
        for (size_t i = 0; i < s->params.size(); ++i) {
            if (i > 0) out << ", ";
            out << s->params[i];
        }
        out << ")\n";
        indentLevel++;
        for (const auto& child : s->body) {
            out << printStmt(*child);
        }
        indentLevel--;
    } else if (auto* s = dynamic_cast<const ReturnStmt*>(&stmt)) {
        out << indent() << "ReturnStmt\n";
        if (s->value) {
            indentLevel++;
            out << printExpr(*s->value);
            indentLevel--;
        }
    } else {
        out << indent() << "UnknownStmt\n";
    }

    return out.str();
}

std::string AstPrinter::printExpr(const Expr& expr) {
    std::ostringstream out;

    if (auto* e = dynamic_cast<const LiteralExpr*>(&expr)) {
        out << indent() << "Literal " << e->stringValue << "\n";
    } else if (auto* e = dynamic_cast<const VariableExpr*>(&expr)) {
        out << indent() << "VariableExpr " << e->name << "\n";
    } else if (auto* e = dynamic_cast<const UnaryExpr*>(&expr)) {
        out << indent() << "UnaryExpr " << opToString(e->op) << "\n";
        indentLevel++;
        out << printExpr(*e->right);
        indentLevel--;
    } else if (auto* e = dynamic_cast<const BinaryExpr*>(&expr)) {
        out << indent() << "BinaryExpr " << opToString(e->op) << "\n";
        indentLevel++;
        out << printExpr(*e->left);
        out << printExpr(*e->right);
        indentLevel--;
    } else if (auto* e = dynamic_cast<const AssignExpr*>(&expr)) {
        out << indent() << "AssignExpr " << e->name << "\n";
        indentLevel++;
        out << printExpr(*e->value);
        indentLevel--;
    } else if (auto* e = dynamic_cast<const CallExpr*>(&expr)) {
        out << indent() << "CallExpr\n";
        indentLevel++;
        out << indent() << "callee:\n";
        indentLevel++;
        out << printExpr(*e->callee);
        indentLevel--;
        out << indent() << "args:\n";
        indentLevel++;
        for (const auto& arg : e->arguments) {
            out << printExpr(*arg);
        }
        indentLevel--;
        indentLevel--;
    } else {
        out << indent() << "UnknownExpr\n";
    }

    return out.str();
}
