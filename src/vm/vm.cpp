#include "vm/vm.hpp"

#include "compiler/opcode.hpp"

#include <iostream>

void VM::reset() {
    stackTop = stack;
    frameCount = 0;
    globals.clear();
    openUpvalues.clear();
    globals["len"] = ConstantValue::makeNative(NativeId::Len);
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
    return frame->closure->function->chunk.constants[readByte(frame)];
}

int VM::currentLine(CallFrame* frame) const {
    auto& code = frame->closure->function->chunk;
    size_t offset = static_cast<size_t>(frame->ip - code.code.data());
    if (offset == 0) {
        return code.lines.empty() ? 0 : code.lines[0];
    }
    offset -= 1;
    if (offset >= code.lines.size()) {
        return 0;
    }
    return code.lines[offset];
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
        case ConstantValue::Type::Closure:
            return a.closure.get() == b.closure.get();
        case ConstantValue::Type::Array:
            return a.array.get() == b.array.get();
        case ConstantValue::Type::Native:
            return a.nativeId == b.nativeId;
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
            FunctionObject* fn = f->closure->function.get();
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

std::shared_ptr<ObjUpvalue> VM::captureUpvalue(ConstantValue* local) {
    for (auto& up : openUpvalues) {
        if (up->location == local) {
            return up;
        }
    }
    auto created = std::make_shared<ObjUpvalue>();
    created->location = local;
    openUpvalues.push_back(created);
    return created;
}

void VM::closeUpvalues(ConstantValue* last) {
    for (int i = static_cast<int>(openUpvalues.size()) - 1; i >= 0; --i) {
        auto& up = openUpvalues[i];
        if (up->location < last) {
            break;
        }
        up->close();
        openUpvalues.erase(openUpvalues.begin() + i);
    }
}

bool VM::call(const std::shared_ptr<ObjClosure>& closure, int argCount) {
    if (argCount != closure->function->arity) {
        runtimeError("Expected " + std::to_string(closure->function->arity) +
                     " arguments but got " + std::to_string(argCount));
        return false;
    }
    if (frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow (too many call frames)");
        return false;
    }

    CallFrame* frame = &frames[frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code.data();
    frame->slots = stackTop - argCount - 1;
    return true;
}

bool VM::callNative(NativeId id, int argCount) {
    if (id == NativeId::Len) {
        if (argCount != 1) {
            runtimeError("len() expects 1 argument");
            return false;
        }
        ConstantValue arg = pop();
        pop();  // native fn itself
        if (arg.type != ConstantValue::Type::Array) {
            runtimeError("len() expects an array");
            return false;
        }
        push(ConstantValue::makeNumber(
            static_cast<long long>(arg.array->elements.size())));
        return true;
    }
    runtimeError("Unknown native function");
    return false;
}

bool VM::callValue(ConstantValue callee, int argCount) {
    if (callee.type == ConstantValue::Type::Closure && callee.closure) {
        return call(callee.closure, argCount);
    }
    if (callee.type == ConstantValue::Type::Native) {
        return callNative(callee.nativeId, argCount);
    }
    runtimeError("Can only call functions");
    return false;
}

InterpretResult VM::run(const std::shared_ptr<FunctionObject>& script) {
    reset();

    auto closure = std::make_shared<ObjClosure>();
    closure->function = script;
    push(ConstantValue::makeClosure(closure));
    if (!call(closure, 0)) {
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

            case OP_GET_UPVALUE: {
                uint8_t slot = readByte(frame);
                push(*frame->closure->upvalues[slot]->get());
                break;
            }
            case OP_SET_UPVALUE: {
                uint8_t slot = readByte(frame);
                *frame->closure->upvalues[slot]->get() = peek(0);
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

            case OP_CLOSURE: {
                ConstantValue fnVal = readConstant(frame);
                auto closure = std::make_shared<ObjClosure>();
                closure->function = fnVal.function;
                int n = static_cast<int>(fnVal.function->upvalues.size());
                for (int i = 0; i < n; ++i) {
                    uint8_t isLocal = readByte(frame);
                    uint8_t index = readByte(frame);
                    if (isLocal) {
                        closure->upvalues.push_back(
                            captureUpvalue(frame->slots + index));
                    } else {
                        closure->upvalues.push_back(
                            frame->closure->upvalues[index]);
                    }
                }
                push(ConstantValue::makeClosure(closure));
                break;
            }

            case OP_CLOSE_UPVALUE:
                closeUpvalues(stackTop - 1);
                pop();
                break;

            case OP_RETURN: {
                ConstantValue result = pop();
                closeUpvalues(frame->slots);
                frameCount--;
                if (frameCount == 0) {
                    pop();
                    return InterpretResult::Ok;
                }
                stackTop = frames[frameCount].slots;
                push(result);
                frame = &frames[frameCount - 1];
                break;
            }

            case OP_BUILD_ARRAY: {
                uint8_t count = readByte(frame);
                auto arr = std::make_shared<ObjArray>();
                arr->elements.resize(count);
                for (int i = count - 1; i >= 0; --i) {
                    arr->elements[i] = pop();
                }
                push(ConstantValue::makeArray(arr));
                break;
            }

            case OP_INDEX_GET: {
                ConstantValue index = pop();
                ConstantValue object = pop();
                if (object.type != ConstantValue::Type::Array) {
                    return runtimeError("Only arrays support indexing");
                }
                if (index.type != ConstantValue::Type::Number) {
                    return runtimeError("Array index must be a number");
                }
                long long i = index.number;
                if (i < 0 ||
                    i >= static_cast<long long>(object.array->elements.size())) {
                    return runtimeError("Array index out of bounds");
                }
                push(object.array->elements[static_cast<size_t>(i)]);
                break;
            }

            case OP_INDEX_SET: {
                ConstantValue value = pop();
                ConstantValue index = pop();
                ConstantValue object = pop();
                if (object.type != ConstantValue::Type::Array) {
                    return runtimeError("Only arrays support indexing");
                }
                if (index.type != ConstantValue::Type::Number) {
                    return runtimeError("Array index must be a number");
                }
                long long i = index.number;
                if (i < 0 ||
                    i >= static_cast<long long>(object.array->elements.size())) {
                    return runtimeError("Array index out of bounds");
                }
                object.array->elements[static_cast<size_t>(i)] = value;
                push(value);
                break;
            }

            default:
                return runtimeError("Unknown opcode");
        }
    }
}
