#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "compiler/opcode.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct FunctionObject;
// Value stored in a chunk's constant pool (compile-time / VM constants).
struct ConstantValue {
    enum class Type { Nil, Number, Boolean, String, Function };

    Type type = Type::Nil;
    long long number = 0;
    bool boolean = false;
    std::string string;
    std::shared_ptr<FunctionObject> function;

    static ConstantValue makeNil() { return ConstantValue{}; }

    static ConstantValue makeNumber(long long n) {  // i will push Nil as a number to represent null
        ConstantValue v;
        v.type = Type::Number;
        v.number = n;
        return v;
    }

    static ConstantValue makeBoolean(bool b) {
        ConstantValue v;
        v.type = Type::Boolean;
        v.boolean = b;
        return v;
    }

    static ConstantValue makeString(std::string s) {
        ConstantValue v;
        v.type = Type::String;
        v.string = std::move(s);
        return v;
    }

    static ConstantValue makeFunction(std::shared_ptr<FunctionObject> fn) {
        ConstantValue v;
        v.type = Type::Function;
        v.function = std::move(fn);
        return v;
    }

    std::string toString() const;
};

// chunk = bytecode buffer, a dynamic array to hold the instructions for the VM to execute.

struct Chunk {
    std::vector<uint8_t> code;
    std::vector<int> lines;
    std::vector<ConstantValue> constants;

    void writeByte(uint8_t byte, int line);
    void writeOp(OpCode op, int line);
    int addConstant(ConstantValue value);

    // Emit jump with placeholder 0xFFFF offset; returns offset of first operand byte.
    int writeJump(OpCode op, int line);
    void patchJump(int offset);

    // Backward jump: emit OP_LOOP with distance from here back to loopStart.
    void writeLoop(int loopStart, int line);
};

struct FunctionObject {
    std::string name;
    int arity = 0;
    Chunk chunk;
};

#endif
