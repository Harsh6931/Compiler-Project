#include "compiler/disassembler.hpp"

#include <iomanip>
#include <sstream>

namespace {

// disassebler work = convert bytecode into human readable format.
// eg. 
int disassembleInstruction(const Chunk& chunk, int offset, std::ostringstream& out) {
    out << std::right << std::setfill('0') << std::setw(4) << offset << " ";
    out << std::setfill(' ');

    if (offset > 0 && chunk.lines[offset] == chunk.lines[offset - 1]) {
        out << "   | ";
    } else {
        out << std::setw(4) << chunk.lines[offset] << " ";
    }

    auto op = static_cast<OpCode>(chunk.code[offset]);
    switch (op) {
        case OP_CONSTANT:
        case OP_GET_GLOBAL:
        case OP_DEFINE_GLOBAL:
        case OP_SET_GLOBAL: {
            uint8_t index = chunk.code[offset + 1];
            out << std::left << std::setw(18) << opcodeName(op) << std::right
                << " " << static_cast<int>(index) << " '"
                << chunk.constants[index].toString() << "'\n";
            return offset + 2;
        }
        case OP_GET_LOCAL:
        case OP_SET_LOCAL:
        case OP_CALL: {
            uint8_t slot = chunk.code[offset + 1];
            out << std::left << std::setw(18) << opcodeName(op) << std::right
                << " " << static_cast<int>(slot) << "\n";
            return offset + 2;
        }
        case OP_JUMP:
        case OP_JUMP_IF_FALSE: {
            uint16_t jump = static_cast<uint16_t>(
                (chunk.code[offset + 1] << 8) | chunk.code[offset + 2]);
            out << std::left << std::setw(18) << opcodeName(op) << std::right
                << " " << offset << " -> " << (offset + 3 + jump) << "\n";
            return offset + 3;
        }
        case OP_LOOP: {
            uint16_t jump = static_cast<uint16_t>(
                (chunk.code[offset + 1] << 8) | chunk.code[offset + 2]);
            out << std::left << std::setw(18) << opcodeName(op) << std::right
                << " " << offset << " -> " << (offset + 3 - jump) << "\n";
            return offset + 3;
        }
        default:
            out << opcodeName(op) << "\n";
            return offset + 1;
    }
}

}  // namespace

std::string disassembleChunk(const Chunk& chunk, const std::string& name) {
    std::ostringstream out;
    out << "== " << name << " ==\n";

    for (int offset = 0; offset < static_cast<int>(chunk.code.size());) {
        offset = disassembleInstruction(chunk, offset, out);
    }

    for (size_t i = 0; i < chunk.constants.size(); ++i) {
        const ConstantValue& c = chunk.constants[i];
        if (c.type == ConstantValue::Type::Function && c.function) {
            out << "\n" << disassembleFunction(*c.function);
        }
    }

    return out.str();
}

std::string disassembleFunction(const FunctionObject& function) {
    std::string name = function.name.empty() ? "<script>" : function.name;
    name += " (" + std::to_string(function.arity) + " params)";
    return disassembleChunk(function.chunk, name);
}
