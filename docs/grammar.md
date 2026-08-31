# Lumen Grammar

Informal BNF for the Lumen toy language. This is the single source of truth
for what the lexer and parser must accept. If a construct is not listed here,
it is out of scope for the current version.

---

## Why this exists

- **Parser blueprint** — each rule maps to recursive-descent code (Stage 2).
- **Removes ambiguity** — precedence (e.g. `*` before `+`) is decided before coding.
- **Test validation** — sample `.lum` programs are checked against these rules.
- **Feature freeze** — if it is not in the BNF, do not implement it yet.

---

## Grammar

```
program         = statement*

statement       = varDecl
                | ifStmt
                | whileStmt
                | block
                | exprStmt
                | returnStmt
                | printStmt
                | functionDecl

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
assignment      = call "=" expression
                | logicalOr
logicalOr       = logicalAnd ( "||" logicalAnd )*
logicalAnd      = equality ( "&&" equality )*
equality        = comparison ( ("==" | "!=") comparison )*
comparison      = term ( ("<" | ">" | "<=" | ">=") term )*
term            = factor ( ("+" | "-") factor )*
factor          = unary ( ("*" | "/") unary )*
unary           = ("-" | "!") unary
                | call
call            = primary ( "(" argumentList? ")" | "[" expression "]" )*
argumentList    = expression ("," expression)*
primary         = NUMBER
                | STRING
                | "true"
                | "false"
                | IDENTIFIER
                | "(" expression ")"
                | "[" (expression ("," expression)*)? "]"
```

Indexing uses `call` postfix forms: `a[i]` and assignment `a[i] = value` (parser accepts identifier or index targets).

---

## Lexical tokens

```
COMMENT         = "//" .* "\n"          (skipped by the lexer)
NUMBER          = [0-9]+
STRING          = '"' .* '"'            (escape sequences later)
IDENTIFIER      = [a-zA-Z_][a-zA-Z0-9_]*
```

### Keywords

`let` `fn` `if` `else` `while` `return` `print` `true` `false`

### Operators and punctuation

`+` `-` `*` `/` `!` `==` `!=` `<` `>` `<=` `>=` `&&` `||` `=`  
`(` `)` `{` `}` `[` `]` `,` `;`

---

## Operator precedence (low → high)

1. Assignment `=`
2. Logical or `||`
3. Logical and `&&`
4. Equality `==` `!=`
5. Comparison `<` `>` `<=` `>=`
6. Term `+` `-`
7. Factor `*` `/`
8. Unary `-` `!`
9. Call / index `f(...)` `a[i]`
10. Primary

---

## Notes

- Values are dynamically typed at runtime: integers, booleans, strings, arrays, and closures.
- Nested `fn` declarations capture enclosing locals (closures / upvalues).
- Builtin `len(array)` returns array length.
- `&&` and `||` short-circuit.
- Single-line comments start with `//` and run to end of line.
