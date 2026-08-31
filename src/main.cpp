#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "ast/ast_printer.hpp"
#include "compiler/compiler.hpp"
#include "compiler/disassembler.hpp"
#include "compiler/optimizer.hpp"
#include "interpreter/interpreter.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "vm/vm.hpp"

// My entry point for the compiler, it call Lexer,parser and parses CLI arguments.

void printTokens(const std::vector<Token>& tokens) {
    for (const auto& tok : tokens) {
        std::cout << tokenTypeToString(tok.type)
                  << " \"" << tok.lexeme << "\" (line " << tok.line
                  << ", col " << tok.column << ")\n";
    }
}

// this will read entire .lum file into a string.
bool readSourceFile(const char* path, std::string& out) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << path << "\n";
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    out = buffer.str();
    return true;
}

// True when braces/parens/brackets are balanced and we are not inside a string.
bool isSourceComplete(const std::string& source) {
    int braces = 0;
    int parens = 0;
    int brackets = 0;
    bool inString = false;

    for (size_t i = 0; i < source.size(); ++i) {
        char c = source[i];
        if (inString) {
            if (c == '\\' && i + 1 < source.size()) {
                ++i;
                continue;
            }
            if (c == '"') {
                inString = false;
            }
            continue;
        }
        if (c == '"') {
            inString = true;
        } else if (c == '{') {
            braces++;
        } else if (c == '}') {
            braces--;
        } else if (c == '(') {
            parens++;
        } else if (c == ')') {
            parens--;
        } else if (c == '[') {
            brackets++;
        } else if (c == ']') {
            brackets--;
        }
    }

    return !inString && braces <= 0 && parens <= 0 && brackets <= 0;
}

bool runSourceOnVm(VM& vm, std::string source, bool optimize) {
    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(tokens, source);
        Program program = parser.parse();
        if (optimize) {
            optimizeProgram(program);
        }
        Compiler compiler;
        auto script = compiler.compile(program);
        return vm.run(script) == InterpretResult::Ok;
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
}

void runRepl() {
    std::cout << "Lumen REPL (Stage 7). Type expressions/statements.\n"
              << "Multi-line input is supported. Ctrl+C or empty EOF to exit.\n";

    VM vm;
    vm.reset();

    std::string buffer;
    std::string line;

    while (true) {
        std::cout << (buffer.empty() ? ">>> " : "... ");
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break;
        }

        if (buffer.empty() && (line == "exit" || line == "quit")) {
            break;
        }

        if (!buffer.empty()) {
            buffer += "\n";
        }
        buffer += line;

        if (!isSourceComplete(buffer)) {
            continue;
        }

        // Skip blank submissions
        bool onlyWs = true;
        for (char c : buffer) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                onlyWs = false;
                break;
            }
        }
        if (onlyWs) {
            buffer.clear();
            continue;
        }

        runSourceOnVm(vm, buffer, true);
        buffer.clear();
    }
}

void printUsage() {
    std::cout << "Lumen — toy language compiler\n"
              << "Usage:\n"
              << "  lumen                      Start REPL\n"
              << "  lumen repl                 Start REPL\n"
              << "  lumen tokenize <file.lum>\n"
              << "  lumen parse <file.lum>\n"
              << "  lumen run <file.lum>\n"
              << "  lumen interpret <file.lum>\n"
              << "  lumen disassemble <file.lum>\n";
}

// For tokenize/parse/run/interpret/disassemble command 3 args mandatory
// other commands have 2+ -> not implemented yet
// argc = arguent count (Number of command-line arguments passed to program.)
// eg. /lumen tokenize test.lum    -> argc = 3 argv[0] = "./lumen" argv[1] = "tokenize"    argv[2] = "test.lum"
// eg. /lumen parse test.lum       -> argc = 3 argv[0] = "./lumen" argv[1] = "parse"       argv[2] = "test.lum"
// eg. /lumen run test.lum         -> argc = 3 argv[0] = "./lumen" argv[1] = "run"         argv[2] = "test.lum"
// eg. /lumen interpret test.lum   -> argc = 3 argv[0] = "./lumen" argv[1] = "interpret"   argv[2] = "test.lum"
// eg. /lumen disassemble test.lum -> argc = 3 argv[0] = "./lumen" argv[1] = "disassemble" argv[2] = "test.lum"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        runRepl();
        return 0;
    }

    std::string command = argv[1];

    if (command == "repl") {
        runRepl();
        return 0;
    }

    if (command == "tokenize" || command == "parse" || command == "run" ||
        command == "interpret" || command == "disassemble") {
        if (argc < 3) {
            std::cerr << "Error: Missing filename.\n";
            return 1;
        }

        std::string source;
        if (!readSourceFile(argv[2], source)) {
            return 1;
        }

        try {
            Lexer lexer(source);
            auto tokens = lexer.tokenize();

            if (command == "tokenize") {
                printTokens(tokens);
                return 0;
            }

            // parse: tokens -> AST
            Parser parser(tokens, source);
            Program program = parser.parse();

            if (command == "parse") {
                // pretty-print the tree (before folding, so tree matches source shape)
                AstPrinter printer;
                std::cout << printer.print(program);
                return 0;
            }

            // Constant folding (Stage 7) for compile / interpret / disassemble
            optimizeProgram(program);

            if (command == "disassemble") {
                // compile AST -> bytecode, then print opcodes (Stage 4)
                Compiler compiler;
                auto script = compiler.compile(program);
                std::cout << disassembleFunction(*script);
                return 0;
            }

            if (command == "interpret") {
                // Stage 3 tree-walking interpreter (correctness oracle)
                Interpreter interpreter;
                interpreter.interpret(program);
                return 0;
            }

            // run: compile AST -> bytecode, then execute on the VM (Stage 5)
            Compiler compiler;
            auto script = compiler.compile(program);
            VM vm;
            vm.reset();
            if (vm.run(script) != InterpretResult::Ok) {
                return 1;
            }
        } catch (const std::runtime_error& e) {
            std::cerr << e.what() << "\n";
            return 1;
        }
    } else {
        std::cerr << "Unknown command '" << command << "'.\n";
        printUsage();
        return 1;
    }

    return 0;
}
