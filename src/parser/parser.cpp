// implementation file of parser class.
//Contains the actual code (logic) for all functions declared in Parser.hpp

#include "parser/parser.hpp"

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

Program Parser::parse() {
    Program program;
    while (!isAtEnd()) {
        program.statements.push_back(declaration());
    }
    return program;
}

// --- token helpers ---

const Token& Parser::peek() const {
    return tokens[current];
}

const Token& Parser::previous() const {
    return tokens[current - 1];
}

bool Parser::isAtEnd() const { //here we add the EOF at the last (more clear on paper)
    return peek().type == TokenType::END_OF_FILE;
}

const Token& Parser::advance() {
    if (!isAtEnd()) {
        current++;
    }
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) {
        return false;
    }
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (!check(type)) {
        return false;
    }
    advance();
    return true;
}

bool Parser::matchAny(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (match(type)) {
            return true;
        }
    }
    return false;
}

const Token& Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {  // TOC logic consume token to move forward in the token stream
        return advance();
    }
    throw error(peek(), message);
}

ParseError Parser::error(const Token& token, const std::string& message) {
    std::string where = token.type == TokenType::END_OF_FILE
                            ? "at end"
                            : "at '" + token.lexeme + "'";
    return ParseError("Parse error at line " + std::to_string(token.line) +
                      " " + where + ": " + message);
}

// --- statements ---

StmtPtr Parser::declaration() {
    if (match(TokenType::FN)) {
        return functionDeclaration();
    }
    if (match(TokenType::LET)) {
        return varDeclaration();
    }
    return statement();
}

StmtPtr Parser::statement() {
    if (match(TokenType::IF)) {
        return ifStatement();
    }
    if (match(TokenType::WHILE)) {
        return whileStatement();
    }
    if (match(TokenType::PRINT)) {
        return printStatement();
    }
    if (match(TokenType::RETURN)) {
        return returnStatement();
    }
    if (check(TokenType::LBRACE)) {
        return blockStatement();
    }
    return expressionStatement();
}

StmtPtr Parser::varDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name after 'let'");
    consume(TokenType::EQ, "Expected '=' after variable name");
    ExprPtr initializer = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    return std::make_unique<VarDeclStmt>(name.lexeme, std::move(initializer), name.line);
}

StmtPtr Parser::functionDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected function name after 'fn'");
    consume(TokenType::LPAREN, "Expected '(' after function name");

    std::vector<std::string> params;
    if (!check(TokenType::RPAREN)) {
        do {
            Token param = consume(TokenType::IDENTIFIER, "Expected parameter name");
            params.push_back(param.lexeme);
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "Expected ')' after parameters");

    consume(TokenType::LBRACE, "Expected '{' before function body");
    std::vector<StmtPtr> body;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        body.push_back(declaration());
    }
    consume(TokenType::RBRACE, "Expected '}' after function body");

    return std::make_unique<FunctionDeclStmt>(
        name.lexeme, std::move(params), std::move(body), name.line);
}

StmtPtr Parser::ifStatement() {
    consume(TokenType::LPAREN, "Expected '(' after 'if'");
    ExprPtr condition = expression();
    consume(TokenType::RPAREN, "Expected ')' after if condition");

    StmtPtr thenBranch = statement();
    StmtPtr elseBranch = nullptr;
    if (match(TokenType::ELSE)) {
        elseBranch = statement();
    }

    return std::make_unique<IfStmt>(
        std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

StmtPtr Parser::whileStatement() {
    consume(TokenType::LPAREN, "Expected '(' after 'while'");
    ExprPtr condition = expression();
    consume(TokenType::RPAREN, "Expected ')' after while condition");
    StmtPtr body = statement();
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

StmtPtr Parser::printStatement() {
    ExprPtr value = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after print value");
    return std::make_unique<PrintStmt>(std::move(value));
}

StmtPtr Parser::returnStatement() {
    Token keyword = previous();
    ExprPtr value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = expression();
    }
    consume(TokenType::SEMICOLON, "Expected ';' after return value");
    return std::make_unique<ReturnStmt>(std::move(value), keyword.line);
}

StmtPtr Parser::blockStatement() {
    consume(TokenType::LBRACE, "Expected '{'");
    std::vector<StmtPtr> statements;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        statements.push_back(declaration());
    }
    consume(TokenType::RBRACE, "Expected '}' after block");
    return std::make_unique<BlockStmt>(std::move(statements));
}

StmtPtr Parser::expressionStatement() {
    ExprPtr expr = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression");
    return std::make_unique<ExprStmt>(std::move(expr));
}

// --- expressions (Pratt / precedence climbing) ---
//
// Precedence (low → high), matching docs/grammar.md:
//   1 assignment (=)
//   2 ||
//   3 &&
//   4 == !=
//   5 < > <= >=
//   6 + -
//   7 * /
//   8 unary / call / primary  (handled in parsePrefix)

ExprPtr Parser::expression() {
    return parsePrecedence(1);
}

int Parser::infixPrecedence(TokenType type) const {
    switch (type) {
        case TokenType::EQ: return 1;
        case TokenType::OR: return 2;
        case TokenType::AND: return 3;
        case TokenType::EQ_EQ:
        case TokenType::BANG_EQ: return 4;
        case TokenType::LESS:
        case TokenType::GREATER:
        case TokenType::LESS_EQ:
        case TokenType::GREATER_EQ: return 5;
        case TokenType::PLUS:
        case TokenType::MINUS: return 6;
        case TokenType::STAR:
        case TokenType::SLASH: return 7;
        default: return -1;  // not an infix operator
    }
}

bool Parser::isRightAssociative(TokenType type) const {
    return type == TokenType::EQ;
}

ExprPtr Parser::parsePrecedence(int minPrecedence) {
    ExprPtr left = parsePrefix();

    while (true) {
        TokenType opType = peek().type;
        int prec = infixPrecedence(opType);
        if (prec < minPrecedence) {
            break;
        }

        Token op = advance();

        // Assignment is special: variable or array index
        if (opType == TokenType::EQ) {
            if (auto* var = dynamic_cast<VariableExpr*>(left.get())) {
                std::string name = var->name;
                int line = var->line;
                ExprPtr value = parsePrecedence(prec);
                left = std::make_unique<AssignExpr>(
                    std::move(name), std::move(value), line);
                continue;
            }
            if (auto* idx = dynamic_cast<IndexExpr*>(left.get())) {
                int line = idx->line;
                ExprPtr object = std::move(idx->object);
                ExprPtr index = std::move(idx->index);
                ExprPtr value = parsePrecedence(prec);
                left = std::make_unique<IndexAssignExpr>(
                    std::move(object), std::move(index), std::move(value), line);
                continue;
            }
            throw error(op, "Invalid assignment target");
        }

        int nextMin = isRightAssociative(opType) ? prec : prec + 1;
        ExprPtr right = parsePrecedence(nextMin);
        left = std::make_unique<BinaryExpr>(
            std::move(left), opType, std::move(right), op.line);
    }

    return left;
}

ExprPtr Parser::parsePrefix() {
    if (match(TokenType::MINUS) || match(TokenType::BANG)) {
        Token op = previous();
        // unary binds tighter than any binary infix
        ExprPtr right = parsePrecedence(8);
        return std::make_unique<UnaryExpr>(op.type, std::move(right), op.line);
    }

    ExprPtr expr;

    if (match(TokenType::NUMBER)) {
        Token tok = previous();
        long long value = std::stoll(tok.lexeme);
        expr = LiteralExpr::number(value, tok.line);
    } else if (match(TokenType::STRING)) {
        Token tok = previous();
        expr = LiteralExpr::string(tok.lexeme, tok.line);
    } else if (match(TokenType::TRUE)) {
        expr = LiteralExpr::boolean(true, previous().line);
    } else if (match(TokenType::FALSE)) {
        expr = LiteralExpr::boolean(false, previous().line);
    } else if (match(TokenType::IDENTIFIER)) {
        Token tok = previous();
        expr = std::make_unique<VariableExpr>(tok.lexeme, tok.line);
    } else if (match(TokenType::LPAREN)) {
        expr = expression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
    } else if (match(TokenType::LBRACKET)) {
        Token start = previous();
        std::vector<ExprPtr> elements;
        if (!check(TokenType::RBRACKET)) {
            do {
                elements.push_back(expression());
            } while (match(TokenType::COMMA));
        }
        consume(TokenType::RBRACKET, "Expected ']' after array elements");
        expr = std::make_unique<ArrayExpr>(std::move(elements), start.line);
    } else {
        throw error(peek(), "Expected expression");
    }

    // Postfix calls and indexing: primary ( "(" args ")" | "[" index "]" )*
    while (true) {
        if (match(TokenType::LPAREN)) {
            expr = finishCall(std::move(expr));
        } else if (match(TokenType::LBRACKET)) {
            ExprPtr index = expression();
            Token rb = consume(TokenType::RBRACKET, "Expected ']' after index");
            expr = std::make_unique<IndexExpr>(
                std::move(expr), std::move(index), rb.line);
        } else {
            break;
        }
    }

    return expr;
}

ExprPtr Parser::finishCall(ExprPtr callee) {
    std::vector<ExprPtr> arguments;
    if (!check(TokenType::RPAREN)) {
        do {
            arguments.push_back(expression());
        } while (match(TokenType::COMMA));
    }
    Token paren = consume(TokenType::RPAREN, "Expected ')' after arguments");
    return std::make_unique<CallExpr>(
        std::move(callee), std::move(arguments), paren.line);
}
