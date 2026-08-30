#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "ast/ast_printer.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

// My entry point for the compiler, it call Lexer,parser and parses CLI arguments.

void printTokens(const std::vector<Token>& tokens) {
    for (const auto& tok : tokens) {
        std::cout << tokenTypeToString(tok.type)
                  << " \"" << tok.lexeme << "\" (line " << tok.line << ")\n";
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

// For tokenize/parse command 3 files mandatory
// other commands have 2+ -> not implemented yet
// argc = arguent count (Number of command-line arguments passed to program.)
// eg. /lumen tokenize test.lum -> argc = 3 argv[0] = "./lumen" argv[1] = "tokenize" argv[2] = "test.lum"
// eg. /lumen parse test.lum    -> argc = 3 argv[0] = "./lumen" argv[1] = "parse"    argv[2] = "test.lum"

int main(int argc, char* argv[]) {
    if (argc < 2) {   // if no arguments are provided, print usage
        std::cout << "Lumen — toy language compiler\n"
                  << "Usage:\n"
                  << "  lumen tokenize <file.lum>\n"
                  << "  lumen parse <file.lum>\n";
        return 0;
    }

    std::string command = argv[1];

    if (command == "tokenize" || command == "parse") {
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

            // parse: tokens -> AST, then pretty-print the tree
            Parser parser(tokens);
            Program program = parser.parse();
            AstPrinter printer;
            std::cout << printer.print(program);
        } catch (const std::runtime_error& e) {
            std::cerr << e.what() << "\n";
            return 1;
        }
    } else {
        std::cout << "Command '" << command << "' not implemented yet.\n";
        return 1;
    }

    return 0;
}
