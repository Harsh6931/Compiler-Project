1. Choosed C++ as toy language.

# Choose All core idea to include 
2. The core features included:
2.1 Data Types: Integers, Booleans, and Strings
What: 123, true/false, "hello".

Why: Forces lexer to handle distinct literal formats (digits, quoted text, keywords) and forces VM to handle different value representations on the stack.

Concept learned: Value representation – how to store different kinds of data in a dynamically typed system.

2.2 Variables: let x = 10;
What: Variable declaration and assignment.

Why: This introduces environments (symbol tables). You must store names and look them up later. It forces you to design scoping rules early.

Concept learned: Binding & Environments – how names map to memory/storage.

2.3 Arithmetic: +, -, *, /
What: Basic binary math operations.

Why: This is the foundation of Pratt parsing (precedence climbing). Multiplication must bind tighter than addition. If you get this wrong, your parser is broken forever.

Concept learned: Operator Precedence & Associativity.

2.4 Comparisons: ==, !=, <, >, <=, >=
What: Equality and relational operators (returning Booleans).

Why: These produce Boolean values, which are distinct from numbers. They force your VM to implement conditional branching (the JUMP_IF_FALSE instruction).

Concept learned: Truthiness & Control-flow predicates.

2.5 Logical Operators: &&, ||, ! (or and/or/not)
What: Combining Boolean expressions.

Why: Unlike arithmetic, these require short-circuiting – i.e., in a && b, if a is false, b must never be evaluated. This requires a unique compilation strategy (jumping over the right-hand side).

Concept learned: Short-circuit evaluation (a surprisingly tricky compiler concept).

2.6  Control Flow: if / else
What: Conditional execution of blocks.

Why: This is where you learn forward jumps and backpatching. When you compile if, you don't yet know where the else or end of the block is until you've parsed ahead—so you must leave a "hole" in the bytecode and patch it later.

Concept learned: Forward jump backpatching.

2.7 Loops: while
What: Repetitive execution.

Why: While if teaches forward jumps, while teaches backward jumps (looping back to an earlier instruction). It also reinforces backpatching for conditional exits.

Concept learned: Backward jumps & infinite-loop prevention in VMs.

2.8  Functions: fn name(params) { ... } and return
What: User-defined subroutines with parameters and return values.

Why: This is the most crucial feature. It forces you to build a call stack (stack frames), manage local variables per invocation, handle return by unwinding the stack, and pass arguments via registers/stack slots.

Concept learned: Call frames, Instruction Pointer management, and the Calling Convention.

2.9  I/O: print
What: Built-in function to output values.

Why: Without this, you cannot verify your program's output in your test files! It also teaches you how to handle native/host interop (calling C++/Java's printf/console.log from inside your VM).

Concept learned: Foreign Function Interface (FFI) - basics.

2.10 Comments & Whitespace: // single-line
What: Skipping // comment and whitespace/newlines.

Why: Technically a lexer feature, but you must include it now. It teaches you to filter input (the lexer's job is to discard irrelevant text so the parser never sees it).

Concept learned: Lexical filtering / Token discarding.

# Lumen Files (.lum)
.lum = Lumen source code files.
Why .lum:
Identifies file type for your CLI (lumen run test.lum).
Enables syntax highlighting in editors.
Distinguishes from .c, .py, .js.

# STAGE 1: LEXICAL ANALYSIS (convert source code into tokens) -TOKENIZATION()

# STAGE 2: SYNTAX ANALYSIS (Parsing)
GOAL = convert PARSE Tokens -> AST

AST =  Abstract Syntax Tree.

What it is: Tree structure representing your code's grammar. Nodes = operations/constructs. Children = operands/parts.

Example: a + b * 2
Tokens: flat list [a, +, b, *, 2]

AST:
    BinaryExpr(+)
    ├── Variable(a)
    └── BinaryExpr(*)
        ├── Variable(b)
        └── Literal(2)

Removes syntax noise (parentheses, semicolons). Captures meaning and operator precedence. Parser creates it; interpreter/compiler consumes it.

