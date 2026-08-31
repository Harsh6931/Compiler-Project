#include "compiler/chunk.hpp"

#include <stdexcept>

std::string ConstantValue::toString() const {
    switch (type) {
        case Type::Nil:
            return "nil";
        case Type::Number:
            return std::to_string(number);
        case Type::Boolean:
            return boolean ? "true" : "false";
        case Type::String:
            return string;
        case Type::Function:
            return "<fn " + function->name + ">";
    }
    return "nil";
}

void Chunk::writeByte(uint8_t byte, int line) {
    code.push_back(byte);
    lines.push_back(line);
}

void Chunk::writeOp(OpCode op, int line) {
    writeByte(static_cast<uint8_t>(op), line);
}

int Chunk::addConstant(ConstantValue value) {
    if (constants.size() >= 256) {
        throw std::runtime_error("Too many constants in one chunk (max 256)");
    }
    constants.push_back(std::move(value));
    return static_cast<int>(constants.size() - 1);
}

int Chunk::writeJump(OpCode op, int line) {  // write a jump instruction and return the offset of the first operand byte.
    writeOp(op, line);                 // used in eg if (condition) { jump to end } else { jump to end }
    writeByte(0xff, line);
    writeByte(0xff, line);
    return static_cast<int>(code.size() - 2);
}

void Chunk::patchJump(int offset) {  // patch the jump instruction with the actual offset.
    int jump = static_cast<int>(code.size()) - offset - 2;  // eg if jump is 10, then offset is 10-2=8
    if (jump > 0xffff) {
        throw std::runtime_error("Jump offset too large");
    }
    code[offset] = static_cast<uint8_t>((jump >> 8) & 0xff);
    code[offset + 1] = static_cast<uint8_t>(jump & 0xff);
}

void Chunk::writeLoop(int loopStart, int line) {
    writeOp(OP_LOOP, line);
    int jump = static_cast<int>(code.size()) - loopStart + 2;
    if (jump > 0xffff) {
        throw std::runtime_error("Loop body too large");
    }
    writeByte(static_cast<uint8_t>((jump >> 8) & 0xff), line);
    writeByte(static_cast<uint8_t>(jump & 0xff), line);
}
