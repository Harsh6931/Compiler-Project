// hpp is C++ header file.
// This file contains the definition of the Token class.
// It is used to represent a token in the lexer,parser, interpreter,compiler,virtual machine,hardware,software,network,database, system,memory.

#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>

// Tokentype represent all type of tokens in language

enum class TokenType {
    // Single-character eg. LPAREN= left paranthesis
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET, SEMICOLON, COMMA,
    PLUS, MINUS, STAR, SLASH, BANG,
    // One or two characters
    EQ, EQ_EQ, BANG_EQ, LESS, GREATER, LESS_EQ, GREATER_EQ,
    AND, OR,
    // Literals
    IDENTIFIER, NUMBER, STRING,
    // Keywords
    LET, FN, IF, ELSE, WHILE, RETURN, PRINT, TRUE, FALSE,
    // Special
    ERROR,
    END_OF_FILE  // not named EOF — that clashes with the C macro EOF
};

// Token class, used to represent a token in the lexer.
struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;  // 1-based start column of the lexeme on that line

    Token(TokenType type, std::string lexeme, int line, int column = 1)
        : type(type),
          lexeme(std::move(lexeme)),
          line(line),
          column(column) {}
};

inline std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::LBRACKET: return "LBRACKET";
        case TokenType::RBRACKET: return "RBRACKET";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::COMMA: return "COMMA";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::STAR: return "STAR";
        case TokenType::SLASH: return "SLASH";
        case TokenType::BANG: return "BANG";  // Logical not
        case TokenType::EQ: return "EQ";
        case TokenType::EQ_EQ: return "EQ_EQ"; // Logical equal
        case TokenType::BANG_EQ: return "BANG_EQ";
        case TokenType::LESS: return "LESS";
        case TokenType::GREATER: return "GREATER";
        case TokenType::LESS_EQ: return "LESS_EQ";
        case TokenType::GREATER_EQ: return "GREATER_EQ";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::STRING: return "STRING";
        case TokenType::LET: return "LET";
        case TokenType::FN: return "FN";  // function keyword
        case TokenType::IF: return "IF";
        case TokenType::ELSE: return "ELSE";
        case TokenType::WHILE: return "WHILE";
        case TokenType::RETURN: return "RETURN";
        case TokenType::PRINT: return "PRINT";
        case TokenType::TRUE: return "TRUE";
        case TokenType::FALSE: return "FALSE";  
        case TokenType::ERROR: return "ERROR";
        case TokenType::END_OF_FILE: return "EOF";  // End of file -> added to know end of file (Buffer & sentinedl concept used this)
        default: return "UNKNOWN";
    }
}

#endif
