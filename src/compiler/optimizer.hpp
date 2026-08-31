#ifndef OPTIMIZER_HPP
#define OPTIMIZER_HPP

#include "ast/ast.hpp"

// Constant folding on the AST (pure literal expressions).
// Dead-code elimination for statements after `return` is done in the compiler.
void optimizeProgram(Program& program);

#endif
