# Lumen Bytecode Opcodes

Stage 4 instruction set. Encoding: **1-byte opcode**; constant/name indexes are **1 byte**; jump/loop operands are **2-byte big-endian unsigned** distances.

Stack effects: `before → after` (top of stack on the right).

## Constants and literals

| Opcode | Operands | Stack | Meaning |
|---|---|---|---|
| `OP_CONSTANT` | 1-byte const index | `[] → [v]` | Push `constants[index]` |
| `OP_NIL` | — | `[] → [nil]` | Push nil |
| `OP_TRUE` | — | `[] → [true]` | Push true |
| `OP_FALSE` | — | `[] → [false]` | Push false |

## Stack / globals

| Opcode | Operands | Stack | Meaning |
|---|---|---|---|
| `OP_POP` | — | `[v] → []` | Discard top |
| `OP_DEFINE_GLOBAL` | 1-byte name const index | `[v] → []` | Bind global to name; pop value |
| `OP_GET_GLOBAL` | 1-byte name const index | `[] → [v]` | Push global |
| `OP_SET_GLOBAL` | 1-byte name const index | `[v] → [v]` | Assign global; leave value |

## Locals (function params / inner lets)

| Opcode | Operands | Stack | Meaning |
|---|---|---|---|
| `OP_GET_LOCAL` | 1-byte slot | `[] → [v]` | Push local at slot |
| `OP_SET_LOCAL` | 1-byte slot | `[v] → [v]` | Set local at slot; leave value |
| `OP_GET_UPVALUE` | 1-byte slot | `[] → [v]` | Push closed-over variable |
| `OP_SET_UPVALUE` | 1-byte slot | `[v] → [v]` | Assign closed-over variable |
| `OP_CLOSE_UPVALUE` | — | `[v] → []` | Close upvalue for stack top, then pop |

Top-level script variables use globals. Inside functions, parameters occupy slots `1 .. arity` (slot 0 is the callee). Captured locals become upvalues.

## Closures and arrays

| Opcode | Operands | Stack | Meaning |
|---|---|---|---|
| `OP_CLOSURE` | const idx + N×(isLocal,index) | `[] → [closure]` | Build closure from function proto |
| `OP_BUILD_ARRAY` | 1-byte count | `[e0..eN] → [array]` | Build array from stack values |
| `OP_INDEX_GET` | — | `[arr,i] → [v]` | Array element get |
| `OP_INDEX_SET` | — | `[arr,i,v] → [v]` | Array element set |

Builtin global `len(array)` is a native function (not an opcode).

## Arithmetic and comparison

| Opcode | Stack | Meaning |
|---|---|---|
| `OP_ADD` | `[a,b] → [a+b]` | Numbers or string concat |
| `OP_SUBTRACT` | `[a,b] → [a-b]` | |
| `OP_MULTIPLY` | `[a,b] → [a*b]` | |
| `OP_DIVIDE` | `[a,b] → [a/b]` | |
| `OP_EQUAL` | `[a,b] → [bool]` | |
| `OP_NOT_EQUAL` | `[a,b] → [bool]` | |
| `OP_GREATER` | `[a,b] → [bool]` | |
| `OP_GREATER_EQUAL` | `[a,b] → [bool]` | |
| `OP_LESS` | `[a,b] → [bool]` | |
| `OP_LESS_EQUAL` | `[a,b] → [bool]` | |
| `OP_NEGATE` | `[a] → [-a]` | |
| `OP_NOT` | `[a] → [!truthy(a)]` | |

## Control flow

| Opcode | Operands | Stack | Meaning |
|---|---|---|---|
| `OP_JUMP` | 2-byte offset | unchanged | IP += offset (forward) |
| `OP_JUMP_IF_FALSE` | 2-byte offset | `[cond] → [cond]` | If falsy, IP += offset (keeps cond for later POP) |
| `OP_LOOP` | 2-byte offset | unchanged | IP -= offset (backward) |

`&&` / `||` are compiled with `JUMP_IF_FALSE` / `JUMP` (short-circuit), not separate opcodes.

## I/O, calls, return

| Opcode | Operands | Stack | Meaning |
|---|---|---|---|
| `OP_PRINT` | — | `[v] → []` | Print value, pop |
| `OP_CALL` | 1-byte arg count | `[callee, args...] → [result]` | Call function |
| `OP_RETURN` | — | `[v] → …` | Return from function with top value |

## Example: `print 1 + 2 * 3;`

```
CONSTANT 1
CONSTANT 2
CONSTANT 3
MULTIPLY      ; stack: [1, 6]
ADD           ; stack: [7]
PRINT
```
