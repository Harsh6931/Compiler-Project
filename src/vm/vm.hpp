#ifndef VM_HPP
#define VM_HPP

#include "compiler/chunk.hpp"

#include <memory>
#include <string>
#include <unordered_map>

enum class InterpretResult {
    Ok,
    RuntimeError,
};

// Stack-based virtual machine that executes compiled FunctionObject bytecode.
class VM {
public:
    InterpretResult run(const std::shared_ptr<FunctionObject>& script);

private:
    static constexpr int STACK_MAX = 256;
    static constexpr int FRAMES_MAX = 64;

    struct CallFrame {
        FunctionObject* function = nullptr;
        uint8_t* ip = nullptr;
        ConstantValue* slots = nullptr;  // points into stack (frame base)
    };

    ConstantValue stack[STACK_MAX];
    ConstantValue* stackTop = stack;
    CallFrame frames[FRAMES_MAX];
    int frameCount = 0;
    std::unordered_map<std::string, ConstantValue> globals;

    void reset();
    void push(ConstantValue value);
    ConstantValue pop();
    ConstantValue peek(int distance) const;

    InterpretResult runFrames();
    bool callValue(ConstantValue callee, int argCount);
    bool call(FunctionObject* function, int argCount);

    uint8_t readByte(CallFrame* frame);
    uint16_t readShort(CallFrame* frame);
    ConstantValue readConstant(CallFrame* frame);

    InterpretResult runtimeError(const std::string& message);
    int currentLine(CallFrame* frame) const;

    static bool isTruthy(const ConstantValue& value);
    static bool valuesEqual(const ConstantValue& a, const ConstantValue& b);
};

#endif
