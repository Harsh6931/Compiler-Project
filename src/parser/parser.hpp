// header file for parser class.
// Declares class methods and members so main.cpp and Parser.cpp know what the Parser looks like.
// without this, main.cpp and Parser.cpp would not know what the Parser looks like.

#ifndef PARSER_HPP
#define PARSER_HPP

#include "ast/ast.hpp"
#include "lexer/token.hpp"

#include <stdexcept>
#include <string>
#include <vector>

class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& message)
        : std::runtime_error(message) {}
};
// Statement goes to Recursive Descent Parsing (the type of top down parsing i studied in compiler design)

// Converts a token stream into an AST (Program).
// Statements: recursive descent. Expressions: Pratt / precedence climbing.
class Parser {
public:
    Parser(const std::vector<Token>& tokens, std::string source);

    Program parse();

private:
    const std::vector<Token>& tokens;
    std::string source;
    size_t current = 0;

    // --- token helpers ---
    const Token& peek() const;
    const Token& previous() const;
    bool isAtEnd() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool matchAny(std::initializer_list<TokenType> types);
    const Token& consume(TokenType type, const std::string& message);
    ParseError error(const Token& token, const std::string& message);

    // --- statements (recursive descent) ---
    StmtPtr declaration();
    StmtPtr statement();
    StmtPtr varDeclaration();
    StmtPtr functionDeclaration();
    StmtPtr ifStatement();
    StmtPtr whileStatement();
    StmtPtr printStatement();
    StmtPtr returnStatement();
    StmtPtr blockStatement();
    StmtPtr expressionStatement();

    // --- expressions (Pratt / precedence climbing) ---
    ExprPtr expression();
    ExprPtr parsePrecedence(int minPrecedence);
    ExprPtr parsePrefix();
    ExprPtr finishCall(ExprPtr callee);

    int infixPrecedence(TokenType type) const;
    bool isRightAssociative(TokenType type) const;
};

#endif
