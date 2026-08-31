#include "vm/vm.hpp"

#include "compiler/opcode.hpp"

#include <iostream>

void VM::reset() {
    stackTop = stack;
    frameCount = 0;
    globals.clear();
}

void VM::push(ConstantValue value) {
    *stackTop++ = std::move(value);
}

ConstantValue VM::pop() {
    return *--stackTop;
}

ConstantValue VM::peek(int distance) const {
    return stackTop[-1 - distance];
}

uint8_t VM::readByte(CallFrame* frame) {
    return *frame->ip++;
}

uint16_t VM::readShort(CallFrame* frame) {
    frame->ip += 2;
    return static_cast<uint16_t>((frame->ip[-2] << 8) | frame->ip[-1]);
}

ConstantValue VM::readConstant(CallFrame* frame) {
    return frame->function->chunk.constants[readByte(frame)];
}

int VM::currentLine(CallFrame* frame) const {
    size_t offset =
        static_cast<size_t>(frame->ip - frame->function->chunk.code.data());
    if (offset == 0) {
        return frame->function->chunk.lines.empty()
                   ? 0
                   : frame->function->chunk.lines[0];
    }
    offset -= 1;
    if (offset >= frame->function->chunk.lines.size()) {
        return 0;
    }
    return frame->function->chunk.lines[offset];
}

bool VM::isTruthy(const ConstantValue& value) {
    if (value.type == ConstantValue::Type::Nil) {
        return false;
    }
    if (value.type == ConstantValue::Type::Boolean) {
        return value.boolean;
    }
    return true;
}

bool VM::valuesEqual(const ConstantValue& a, const ConstantValue& b) {
    if (a.type != b.type) {
        return false;
    }
    switch (a.type) {
        case ConstantValue::Type::Nil:
            return true;
        case ConstantValue::Type::Number:
            return a.number == b.number;
        case ConstantValue::Type::Boolean:
            return a.boolean == b.boolean;
        case ConstantValue::Type::String:
            return a.string == b.string;
        case ConstantValue::Type::Function:
            return a.function.get() == b.function.get();
    }
    return false;
}

InterpretResult VM::runtimeError(const std::string& message) {
    if (frameCount > 0) {
        CallFrame* frame = &frames[frameCount - 1];
        std::cerr << "Runtime error at line " << currentLine(frame) << ": "
                  << message << "\n";

        for (int i = frameCount - 1; i >= 0; --i) {
            CallFrame* f = &frames[i];
            FunctionObject* fn = f->function;
            size_t instruction =
                static_cast<size_t>(f->ip - fn->chunk.code.data());
            if (instruction > 0) {
                instruction -= 1;
            }
            int line = 0;
            if (instruction < fn->chunk.lines.size()) {
                line = fn->chunk.lines[instruction];
            }
            std::cerr << "[line " << line << "] in ";
            if (fn->name == "<script>") {
                std::cerr << "script\n";
            } else {
                std::cerr << fn->name << "()\n";
            }
        }
    } else {
        std::cerr << "Runtime error: " << message << "\n";
    }

    reset();
    return InterpretResult::RuntimeError;
}

bool VM::call(FunctionObject* function, int argCount) {
    if (argCount != function->arity) {
        runtimeError("Expected " + std::to_string(function->arity) +
                     " arguments but got " + std::to_string(argCount));
        return false;
    }
    if (frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow (too many call frames)");
        return false;
    }

    CallFrame* frame = &frames[frameCount++];
    frame->function = function;
    frame->ip = function->chunk.code.data();
    frame->slots = stackTop - argCount - 1;
    return true;
}

bool VM::callValue(ConstantValue callee, int argCount) {
    if (callee.type == ConstantValue::Type::Function && callee.function) {
        return call(callee.function.get(), argCount);
    }
    runtimeError("Can only call functions");
    return false;
}

InterpretResult VM::run(const std::shared_ptr<FunctionObject>& script) {
    reset();

    push(ConstantValue::makeFunction(script));
    if (!call(script.get(), 0)) {
        return InterpretResult::RuntimeError;
    }

    return runFrames();
}

InterpretResult VM::runFrames() {
    CallFrame* frame = &frames[frameCount - 1];

    for (;;) {
        OpCode instruction = static_cast<OpCode>(readByte(frame));

        switch (instruction) {
            case OP_CONSTANT:
                push(readConstant(frame));
                break;

            case OP_NIL:
                push(ConstantValue::makeNil());
                break;

            case OP_TRUE:
                push(ConstantValue::makeBoolean(true));
                break;

            case OP_FALSE:
                push(ConstantValue::makeBoolean(false));
                break;

            case OP_POP:
                pop();
                break;

            case OP_GET_LOCAL: {
                uint8_t slot = readByte(frame);
                push(frame->slots[slot]);
                break;
            }

            case OP_SET_LOCAL: {
                uint8_t slot = readByte(frame);
                frame->slots[slot] = peek(0);
                break;
            }

            case OP_GET_GLOBAL: {
                ConstantValue nameVal = readConstant(frame);
                auto it = globals.find(nameVal.string);
                if (it == globals.end()) {
                    return runtimeError("Undefined variable '" + nameVal.string +
                                        "'");
                }
                push(it->second);
                break;
            }

            case OP_DEFINE_GLOBAL: {
                ConstantValue nameVal = readConstant(frame);
                globals[nameVal.string] = peek(0);
                pop();
                break;
            }

            case OP_SET_GLOBAL: {
                ConstantValue nameVal = readConstant(frame);
                auto it = globals.find(nameVal.string);
                if (it == globals.end()) {
                    return runtimeError("Undefined variable '" + nameVal.string +
                                        "'");
                }
                it->second = peek(0);
                break;
            }

            case OP_EQUAL: {
                ConstantValue b = pop();
                ConstantValue a = pop();
                push(ConstantValue::makeBoolean(valuesEqual(a, b)));
                break;
            }

            case OP_NOT_EQUAL: {
                ConstantValue b = pop();
                ConstantValue a = pop();
                push(ConstantValue::makeBoolean(!valuesEqual(a, b)));
                break;
            }

            case OP_GREATER: {
                if (peek(0).type != ConstantValue::Type::Number ||
                    peek(1).type != ConstantValue::Type::Number) {
                    return runtimeError("Operands must be numbers");
                }
                ConstantValue b = pop();
                ConstantValue a = pop();
                push(ConstantValue::makeBoolean(a.number > b.number));
                break;
            }

            case OP_GREATER_EQUAL: {
                if (peek(0).type != ConstantValue::Type::Number ||
                    peek(1).type != ConstantValue::Type::Number) {
                    return runtimeError("Operands must be numbers");
                }
                ConstantValue b = pop();
                ConstantValue a = pop();
                push(ConstantValue::makeBoolean(a.number >= b.number));
                break;
            }

            case OP_LESS: {
                if (peek(0).type != ConstantValue::Type::Number ||
                    peek(1).type != ConstantValue::Type::Number) {
                    return runtimeError("Operands must be numbers");
                }
                ConstantValue b = pop();
                ConstantValue a = pop();
                push(ConstantValue::makeBoolean(a.number < b.number));
                break;
            }

            case OP_LESS_EQUAL: {
                if (peek(0).type != ConstantValue::Type::Number ||
                    peek(1).type != ConstantValue::Type::Number) {
                    return runtimeError("Operands must be numbers");
                }
                ConstantValue b = pop();
                ConstantValue a = pop();
                push(ConstantValue::makeBoolean(a.number <= b.number));
                break;
            }

            case OP_ADD: {
                if (peek(0).type == ConstantValue::Type::String &&
                    peek(1).type == ConstantValue::Type::String) {
                    ConstantValue b = pop();
                    ConstantValue a = pop();
                    push(ConstantValue::makeString(a.string + b.string));
                } else if (peek(0).type == ConstantValue::Type::Number &&
                           peek(1).type == ConstantValue::Type::Number) {
                    ConstantValue b = pop();
                    ConstantValue a = pop();
                    push(ConstantValue::makeNumber(a.number + b.number));
                } else {
                    return runtimeError(
                        "Operands of '+' must be two numbers or two strings");
                }
                break;
            }

            case OP_SUBTRACT: {
                if (peek(0).type != ConstantValue::Type::Number ||
                    peek(1).type != ConstantValue::Type::Number) {
                    return runtimeError("Operands must be numbers");
                }
                ConstantValue b = pop();
                ConstantValue a = pop();
                push(ConstantValue::makeNumber(a.number - b.number));
                break;
            }

            case OP_MULTIPLY: {
                if (peek(0).type != ConstantValue::Type::Number ||
                    peek(1).type != ConstantValue::Type::Number) {
                    return runtimeError("Operands must be numbers");
                }
                ConstantValue b = pop();
                ConstantValue a = pop();
                push(ConstantValue::makeNumber(a.number * b.number));
                break;
            }

            case OP_DIVIDE: {
                if (peek(0).type != ConstantValue::Type::Number ||
                    peek(1).type != ConstantValue::Type::Number) {
                    return runtimeError("Operands must be numbers");
                }
                ConstantValue b = pop();
                ConstantValue a = pop();
                if (b.number == 0) {
                    return runtimeError("Division by zero");
                }
                push(ConstantValue::makeNumber(a.number / b.number));
                break;
            }

            case OP_NOT:
                push(ConstantValue::makeBoolean(!isTruthy(pop())));
                break;

            case OP_NEGATE: {
                if (peek(0).type != ConstantValue::Type::Number) {
                    return runtimeError("Operand must be a number");
                }
                push(ConstantValue::makeNumber(-pop().number));
                break;
            }

            case OP_PRINT:
                std::cout << pop().toString() << "\n";
                break;

            case OP_JUMP: {
                uint16_t offset = readShort(frame);
                frame->ip += offset;
                break;
            }

            case OP_JUMP_IF_FALSE: {
                uint16_t offset = readShort(frame);
                if (!isTruthy(peek(0))) {
                    frame->ip += offset;
                }
                break;
            }

            case OP_LOOP: {
                uint16_t offset = readShort(frame);
                frame->ip -= offset;
                break;
            }

            case OP_CALL: {
                int argCount = readByte(frame);
                if (!callValue(peek(argCount), argCount)) {
                    return InterpretResult::RuntimeError;
                }
                frame = &frames[frameCount - 1];
                break;
            }

            case OP_RETURN: {
                ConstantValue result = pop();
                frameCount--;
                if (frameCount == 0) {
                    pop();  // pop script function
                    return InterpretResult::Ok;
                }

                stackTop = frames[frameCount].slots;
                push(result);
                frame = &frames[frameCount - 1];
                break;
            }

            default:
                return runtimeError("Unknown opcode");
        }
    }
}
