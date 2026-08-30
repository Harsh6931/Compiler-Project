# BNF (Backus-Naur Form) = syntax notation. 
# Defines valid syntax
Based on language core features decided by me.
Include it because:

Parser blueprint – you translate each rule directly into recursive-descent code (Stage 2).
Removes ambiguity – forces you to decide precedence (e.g., * before +) before coding.
Tests validation – you compare parser output against these rules to find bugs.
Single source of truth – prevents feature creep; if it's not in BNF, don't implement it.

Without BNF, you code blind.

program         = statement*

statement       = varDecl | ifStmt | whileStmt | block | exprStmt | returnStmt | printStmt | functionDecl

varDecl         = "let" IDENTIFIER "=" expression ";"
ifStmt          = "if" "(" expression ")" statement ("else" statement)?
whileStmt       = "while" "(" expression ")" statement
block           = "{" statement* "}"
exprStmt        = expression ";"
returnStmt      = "return" expression? ";"
printStmt       = "print" expression ";"
functionDecl    = "fn" IDENTIFIER "(" parameterList? ")" block
parameterList   = IDENTIFIER ("," IDENTIFIER)*

expression      = assignment
assignment      = IDENTIFIER "=" expression | logicalOr
logicalOr       = logicalAnd ( "||" logicalAnd )*
logicalAnd      = equality ( "&&" equality )*
equality        = comparison ( ("==" | "!=") comparison )*
comparison      = term ( ("<" | ">" | "<=" | ">=") term )*
term            = factor ( ("+" | "-") factor )*
factor          = unary ( ("*" | "/") unary )*
unary           = ("-" | "!" ) unary | call
call            = primary ( "(" argumentList? ")" )*
argumentList    = expression ("," expression)*
primary         = NUMBER | STRING | "true" | "false" | IDENTIFIER | "(" expression ")"

COMMENT         = "//" .* "\n"   (skipped in lexer)
NUMBER          = [0-9]+
STRING          = '"' .* '"'     (handle escapes later)
IDENTIFIER      = [a-zA-Z_][a-zA-Z0-9_]*