#ifndef VM_HPP
#define VM_HPP

#include "compiler/chunk.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

enum class InterpretResult {
    Ok,
    RuntimeError,
};

class VM {
public:
    VM();

    // Clear stack/frames and re-init natives + empty globals.
    void reset();

    // Execute a script chunk. Does not clear globals (REPL-safe).
    // Call reset() first for a fresh file run.
    InterpretResult run(const std::shared_ptr<FunctionObject>& script);

private:
    static constexpr int STACK_MAX = 256;
    static constexpr int FRAMES_MAX = 64;

    struct CallFrame {
        std::shared_ptr<ObjClosure> closure;
        uint8_t* ip = nullptr;
        ConstantValue* slots = nullptr;
    };

    ConstantValue stack[STACK_MAX];
    ConstantValue* stackTop = stack;
    CallFrame frames[FRAMES_MAX];
    int frameCount = 0;
    std::unordered_map<std::string, ConstantValue> globals;
    std::vector<std::shared_ptr<ObjUpvalue>> openUpvalues;

    void push(ConstantValue value);
    ConstantValue pop();
    ConstantValue peek(int distance) const;

    InterpretResult runFrames();
    bool callValue(ConstantValue callee, int argCount);
    bool call(const std::shared_ptr<ObjClosure>& closure, int argCount);
    bool callNative(NativeId id, int argCount);

    std::shared_ptr<ObjUpvalue> captureUpvalue(ConstantValue* local);
    void closeUpvalues(ConstantValue* last);

    uint8_t readByte(CallFrame* frame);
    uint16_t readShort(CallFrame* frame);
    ConstantValue readConstant(CallFrame* frame);

    InterpretResult runtimeError(const std::string& message);
    int currentLine(CallFrame* frame) const;

    static bool isTruthy(const ConstantValue& value);
    static bool valuesEqual(const ConstantValue& a, const ConstantValue& b);
};

#endif
