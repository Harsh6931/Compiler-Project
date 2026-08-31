#ifndef DISASSEMBLER_HPP
#define DISASSEMBLER_HPP

#include "compiler/chunk.hpp"

#include <string>

// Pretty-prints bytecode for review (Stage 4 checkpoint).
std::string disassembleChunk(const Chunk& chunk, const std::string& name);
std::string disassembleFunction(const FunctionObject& function);

#endif
