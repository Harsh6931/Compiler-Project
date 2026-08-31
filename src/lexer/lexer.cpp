// implementation file of lexer class.
//Contains the actual code (logic) for all functions declared in Lexer.hpp

#include "lexer.hpp"

#include "util/error_format.hpp"

#include <cctype>
#include <stdexcept>
#include <unordered_map>

Lexer::Lexer(const std::string& source)
    : source(source), position(0), line(1), column(1) {}

std::vector<Token> Lexer::tokenize() {  // run loop to read characters and tokenize the source code completely
    while (position < source.length()) {
        char c = peek(); // get current character

        if (std::isspace(static_cast<unsigned char>(c))) {
            skipWhitespace();
            continue;
        }

        // '/' is either the start of // comment or the division operator
        if (c == '/') {
            if (position + 1 < source.length() && source[position + 1] == '/') {
                skipComment();
                continue;
            }
            int startCol = column;
            addToken(TokenType::SLASH, std::string(1, advance()), startCol);
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            readNumber();
            continue;
        }

        if (c == '"') {
            readString();
            continue;
        }

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            readIdentifier();
            continue;
        }

        // if its not a number, string, identifier, or comment, its an operator or punctuation
        // Operators and punctuation — always consume the first character,
        // then optionally match a second character for two-char tokens.
        int startCol = column;
        switch (c) {
            case '(':
                addToken(TokenType::LPAREN, std::string(1, advance()), startCol);
                break;
            case ')':
                addToken(TokenType::RPAREN, std::string(1, advance()), startCol);
                break;
            case '{':
                addToken(TokenType::LBRACE, std::string(1, advance()), startCol);
                break;
            case '}':
                addToken(TokenType::RBRACE, std::string(1, advance()), startCol);
                break;
            case '[':
                addToken(TokenType::LBRACKET, std::string(1, advance()), startCol);
                break;
            case ']':
                addToken(TokenType::RBRACKET, std::string(1, advance()), startCol);
                break;
            case ';':
                addToken(TokenType::SEMICOLON, std::string(1, advance()), startCol);
                break;
            case ',':
                addToken(TokenType::COMMA, std::string(1, advance()), startCol);
                break;
            case '+':
                addToken(TokenType::PLUS, std::string(1, advance()), startCol);
                break;
            case '-':
                addToken(TokenType::MINUS, std::string(1, advance()), startCol);
                break;
            case '*':
                addToken(TokenType::STAR, std::string(1, advance()), startCol);
                break;
            case '!':
                advance();
                if (match('=')) {
                    addToken(TokenType::BANG_EQ, "!=", startCol);
                } else {
                    addToken(TokenType::BANG, "!", startCol);
                }
                break;
            case '=':
                advance();
                if (match('=')) {
                    addToken(TokenType::EQ_EQ, "==", startCol);
                } else {
                    addToken(TokenType::EQ, "=", startCol);
                }
                break;
            case '<':
                advance();
                if (match('=')) {
                    addToken(TokenType::LESS_EQ, "<=", startCol);
                } else {
                    addToken(TokenType::LESS, "<", startCol);
                }
                break;
            case '>':
                advance();
                if (match('=')) {
                    addToken(TokenType::GREATER_EQ, ">=", startCol);
                } else {
                    addToken(TokenType::GREATER, ">", startCol);
                }
                break;
            case '&':
                advance();
                if (match('&')) {
                    addToken(TokenType::AND, "&&", startCol);
                } else {
                    error("Expected '&' after '&' for '&&'", startCol);
                }
                break;
            case '|':
                advance();
                if (match('|')) {
                    addToken(TokenType::OR, "||", startCol);
                } else {
                    error("Expected '|' after '|' for '||'", startCol);
                }
                break;
            default:
                error("Unexpected character: " + std::string(1, c), startCol);
                advance();
                break;
        }
    }

    tokens.emplace_back(TokenType::END_OF_FILE, "", line, column);
    return tokens;
}

char Lexer::peek() const {
    if (position >= source.length()) {
        return '\0';
    }
    return source[position];
}

char Lexer::advance() {
    char c = source[position++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

bool Lexer::match(char expected) {
    if (peek() != expected) {
        return false;
    }
    advance();
    return true;
}

void Lexer::skipWhitespace() {
    while (std::isspace(static_cast<unsigned char>(peek()))) {
        advance();
    }
}

void Lexer::skipComment() {
    while (peek() != '\0' && peek() != '\n') {
        advance();
    }
}

void Lexer::readNumber() {
    int startCol = column;
    std::string num;
    while (std::isdigit(static_cast<unsigned char>(peek()))) {
        num += advance();
    }
    addToken(TokenType::NUMBER, num, startCol);
}

void Lexer::readString() {
    int startCol = column;
    advance();  // opening "
    std::string str;

    // how to handle line functions eg n,tab
    
    while (peek() != '"' && peek() != '\0') {  
        if (peek() == '\\') {   
            advance();
            switch (peek()) {
                case 'n':
                    str += '\n';
                    advance();
                    break;
                case 't':
                    str += '\t';
                    advance();
                    break;
                case '\\':
                    str += '\\';
                    advance();
                    break;
                case '"':
                    str += '"';
                    advance();
                    break;
                default:
                    error("Invalid escape sequence", column);
                    return;
            }
        } else {
            str += advance();
        }
    }

    if (peek() == '\0') {
        error("Unterminated string", startCol);
        return;
    }

    advance();  // closing "
    addToken(TokenType::STRING, str, startCol);
}

void Lexer::readIdentifier() {
    int startCol = column;
    std::string id;
    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') {
        id += advance();
    }

    static const std::unordered_map<std::string, TokenType> keywords = {
        {"let", TokenType::LET},
        {"fn", TokenType::FN},
        {"if", TokenType::IF},
        {"else", TokenType::ELSE},
        {"while", TokenType::WHILE},
        {"return", TokenType::RETURN},
        {"print", TokenType::PRINT},
        {"true", TokenType::TRUE},
        {"false", TokenType::FALSE},
    };

    auto it = keywords.find(id);
    if (it != keywords.end()) {
        addToken(it->second, id, startCol);
    } else {
        addToken(TokenType::IDENTIFIER, id, startCol);
    }
}

void Lexer::addToken(TokenType type, const std::string& lexeme, int startColumn) {
    tokens.emplace_back(type, lexeme, line, startColumn);
}

void Lexer::error(const std::string& message, int errorColumn) {
    tokens.emplace_back(TokenType::ERROR, message, line, errorColumn);
    throw std::runtime_error(
        formatCaretError(source, line, errorColumn, "Lexer error", message));
}
