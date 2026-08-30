// header file for lexer class.
// Declares class methods and members so main.cpp and Lexer.cpp know what the Lexer looks like.
// without this, main.cpp and Lexer.cpp would not know what the Lexer looks like.

#ifndef LEXER_HPP
#define LEXER_HPP

#include "token.hpp"
#include <string>
#include <vector>

// Lexer class, used to tokenize the source code.
class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    std::string source;
    size_t position;  // current position in the source code
    int line;  // current line number in the source code
    std::vector<Token> tokens;

    // methods to tokenize the source code( defination is lexer.cpp)
    char peek() const;
    char advance();
    bool match(char expected);
    void skipWhitespace();
    void skipComment();
    void readNumber();
    void readString();
    void readIdentifier();
    void addToken(TokenType type, const std::string& lexeme);
    void error(const std::string& message);
};

#endif
