#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "compiler/opcode.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct FunctionObject;
struct ObjClosure;
struct ObjUpvalue;
struct ObjArray;

enum class NativeId { Len };

struct UpvalueDesc {
    bool isLocal = false;
    uint8_t index = 0;
};

// Value stored in a chunk's constant pool (compile-time / VM constants).
struct ConstantValue {
    enum class Type { Nil, Number, Boolean, String, Function, Closure, Array, Native };

    Type type = Type::Nil;
    long long number = 0;
    bool boolean = false;
    std::string string;
    std::shared_ptr<FunctionObject> function;
    std::shared_ptr<ObjClosure> closure;
    std::shared_ptr<ObjArray> array;
    NativeId nativeId = NativeId::Len;

    static ConstantValue makeNil() { return ConstantValue{}; }

    static ConstantValue makeNumber(long long n) {
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

    static ConstantValue makeClosure(std::shared_ptr<ObjClosure> c) {
        ConstantValue v;
        v.type = Type::Closure;
        v.closure = std::move(c);
        return v;
    }

    static ConstantValue makeArray(std::shared_ptr<ObjArray> a) {
        ConstantValue v;
        v.type = Type::Array;
        v.array = std::move(a);
        return v;
    }

    static ConstantValue makeNative(NativeId id) {
        ConstantValue v;
        v.type = Type::Native;
        v.nativeId = id;
        return v;
    }

    std::string toString() const;
};

struct ObjUpvalue {
    ConstantValue* location = nullptr;
    ConstantValue closed;
    bool isClosed = false;

    ConstantValue* get() { return isClosed ? &closed : location; }

    void close() {
        if (!isClosed && location) {
            closed = *location;
            isClosed = true;
            location = &closed;
        }
    }
};

struct ObjClosure {
    std::shared_ptr<FunctionObject> function;
    std::vector<std::shared_ptr<ObjUpvalue>> upvalues;
};

struct ObjArray {
    std::vector<ConstantValue> elements;
};

struct Chunk {
    std::vector<uint8_t> code;
    std::vector<int> lines;
    std::vector<ConstantValue> constants;

    void writeByte(uint8_t byte, int line);
    void writeOp(OpCode op, int line);
    int addConstant(ConstantValue value);
    int writeJump(OpCode op, int line);
    void patchJump(int offset);
    void writeLoop(int loopStart, int line);
};

struct FunctionObject {
    std::string name;
    int arity = 0;
    Chunk chunk;
    std::vector<UpvalueDesc> upvalues;
};

#endif
