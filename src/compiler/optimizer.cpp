#include "compiler/optimizer.hpp"

#include "lexer/token.hpp"

#include <utility>

namespace {

ExprPtr foldExpr(ExprPtr expr);

bool isNumberLit(const Expr* e, long long& out) {
    auto* lit = dynamic_cast<const LiteralExpr*>(e);
    if (!lit || lit->kind != LiteralExpr::Kind::Number) {
        return false;
    }
    out = lit->numberValue;
    return true;
}

bool isBoolLit(const Expr* e, bool& out) {
    auto* lit = dynamic_cast<const LiteralExpr*>(e);
    if (!lit || lit->kind != LiteralExpr::Kind::Boolean) {
        return false;
    }
    out = lit->boolValue;
    return true;
}

bool isStringLit(const Expr* e, std::string& out) {
    auto* lit = dynamic_cast<const LiteralExpr*>(e);
    if (!lit || lit->kind != LiteralExpr::Kind::String) {
        return false;
    }
    out = lit->stringValue;
    return true;
}

ExprPtr foldUnary(UnaryExpr* e) {
    e->right = foldExpr(std::move(e->right));

    long long n;
    bool b;
    if (e->op == TokenType::MINUS && isNumberLit(e->right.get(), n)) {
        return LiteralExpr::number(-n, e->line);
    }
    if (e->op == TokenType::BANG && isBoolLit(e->right.get(), b)) {
        return LiteralExpr::boolean(!b, e->line);
    }
    if (e->op == TokenType::BANG && isNumberLit(e->right.get(), n)) {
        // numbers are truthy; !n => false (matches VM isTruthy)
        return LiteralExpr::boolean(false, e->line);
    }
    return nullptr;
}

ExprPtr foldBinary(BinaryExpr* e) {
    e->left = foldExpr(std::move(e->left));
    e->right = foldExpr(std::move(e->right));

    // Short-circuit logical folding when left is a known bool
    bool lb;
    if (e->op == TokenType::AND && isBoolLit(e->left.get(), lb)) {
        if (!lb) {
            return LiteralExpr::boolean(false, e->line);
        }
        return std::move(e->right);
    }
    if (e->op == TokenType::OR && isBoolLit(e->left.get(), lb)) {
        if (lb) {
            return LiteralExpr::boolean(true, e->line);
        }
        return std::move(e->right);
    }

    long long ln, rn;
    bool leftNum = isNumberLit(e->left.get(), ln);
    bool rightNum = isNumberLit(e->right.get(), rn);

    if (leftNum && rightNum) {
        switch (e->op) {
            case TokenType::PLUS:
                return LiteralExpr::number(ln + rn, e->line);
            case TokenType::MINUS:
                return LiteralExpr::number(ln - rn, e->line);
            case TokenType::STAR:
                return LiteralExpr::number(ln * rn, e->line);
            case TokenType::SLASH:
                if (rn == 0) {
                    return nullptr;  // leave for runtime error
                }
                return LiteralExpr::number(ln / rn, e->line);
            case TokenType::EQ_EQ:
                return LiteralExpr::boolean(ln == rn, e->line);
            case TokenType::BANG_EQ:
                return LiteralExpr::boolean(ln != rn, e->line);
            case TokenType::LESS:
                return LiteralExpr::boolean(ln < rn, e->line);
            case TokenType::LESS_EQ:
                return LiteralExpr::boolean(ln <= rn, e->line);
            case TokenType::GREATER:
                return LiteralExpr::boolean(ln > rn, e->line);
            case TokenType::GREATER_EQ:
                return LiteralExpr::boolean(ln >= rn, e->line);
            default:
                break;
        }
    }

    std::string ls, rs;
    if (e->op == TokenType::PLUS && isStringLit(e->left.get(), ls) &&
        isStringLit(e->right.get(), rs)) {
        return LiteralExpr::string(ls + rs, e->line);
    }

    bool lbb, rbb;
    if (isBoolLit(e->left.get(), lbb) && isBoolLit(e->right.get(), rbb)) {
        if (e->op == TokenType::EQ_EQ) {
            return LiteralExpr::boolean(lbb == rbb, e->line);
        }
        if (e->op == TokenType::BANG_EQ) {
            return LiteralExpr::boolean(lbb != rbb, e->line);
        }
    }

    return nullptr;
}

ExprPtr foldExpr(ExprPtr expr) {
    if (!expr) {
        return expr;
    }

    if (auto* e = dynamic_cast<UnaryExpr*>(expr.get())) {
        if (auto folded = foldUnary(e)) {
            return folded;
        }
        return expr;
    }

    if (auto* e = dynamic_cast<BinaryExpr*>(expr.get())) {
        if (auto folded = foldBinary(e)) {
            return folded;
        }
        return expr;
    }

    if (auto* e = dynamic_cast<AssignExpr*>(expr.get())) {
        e->value = foldExpr(std::move(e->value));
        return expr;
    }

    if (auto* e = dynamic_cast<CallExpr*>(expr.get())) {
        e->callee = foldExpr(std::move(e->callee));
        for (auto& arg : e->arguments) {
            arg = foldExpr(std::move(arg));
        }
        return expr;
    }

    if (auto* e = dynamic_cast<ArrayExpr*>(expr.get())) {
        for (auto& el : e->elements) {
            el = foldExpr(std::move(el));
        }
        return expr;
    }

    if (auto* e = dynamic_cast<IndexExpr*>(expr.get())) {
        e->object = foldExpr(std::move(e->object));
        e->index = foldExpr(std::move(e->index));
        return expr;
    }

    if (auto* e = dynamic_cast<IndexAssignExpr*>(expr.get())) {
        e->object = foldExpr(std::move(e->object));
        e->index = foldExpr(std::move(e->index));
        e->value = foldExpr(std::move(e->value));
        return expr;
    }

    return expr;
}

void foldStmt(StmtPtr& stmt);

void foldStmtList(std::vector<StmtPtr>& statements) {
    for (auto& stmt : statements) {
        foldStmt(stmt);
    }
}

void foldStmt(StmtPtr& stmt) {
    if (!stmt) {
        return;
    }

    if (auto* s = dynamic_cast<ExprStmt*>(stmt.get())) {
        s->expression = foldExpr(std::move(s->expression));
        return;
    }
    if (auto* s = dynamic_cast<PrintStmt*>(stmt.get())) {
        s->expression = foldExpr(std::move(s->expression));
        return;
    }
    if (auto* s = dynamic_cast<VarDeclStmt*>(stmt.get())) {
        s->initializer = foldExpr(std::move(s->initializer));
        return;
    }
    if (auto* s = dynamic_cast<BlockStmt*>(stmt.get())) {
        foldStmtList(s->statements);
        return;
    }
    if (auto* s = dynamic_cast<IfStmt*>(stmt.get())) {
        s->condition = foldExpr(std::move(s->condition));
        foldStmt(s->thenBranch);
        if (s->elseBranch) {
            foldStmt(s->elseBranch);
        }
        return;
    }
    if (auto* s = dynamic_cast<WhileStmt*>(stmt.get())) {
        s->condition = foldExpr(std::move(s->condition));
        foldStmt(s->body);
        return;
    }
    if (auto* s = dynamic_cast<FunctionDeclStmt*>(stmt.get())) {
        foldStmtList(s->body);
        return;
    }
    if (auto* s = dynamic_cast<ReturnStmt*>(stmt.get())) {
        if (s->value) {
            s->value = foldExpr(std::move(s->value));
        }
        return;
    }
}

}  // namespace

void optimizeProgram(Program& program) {
    foldStmtList(program.statements);
}
