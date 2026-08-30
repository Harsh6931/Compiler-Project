#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "lexer/lexer.hpp"

// My entry point for the compiler, it call Lexer,parser and parses CLI arguments.

void printTokens(const std::vector<Token>& tokens) {
    for (const auto& tok : tokens) {
        std::cout << tokenTypeToString(tok.type)
                  << " \"" << tok.lexeme << "\" (line " << tok.line << ")\n";
    }
}

// For tokenize command 3 files mandatory 
// other commands have 2+ -> print not implemented 
// argc = arguent count (Number of command-line arguments passed to program.)
//eg . /lumen tokenize test.lum -> argc = 3 argv[0] = "./lumen" argv[1] = "tokenize" argv[2] = "test.lum"

int main(int argc, char* argv[]) {
    if (argc < 2) {   // if no arguments are provided, print usage
        std::cout << "Lumen — toy language compiler\n"
                  << "Usage: lumen tokenize <file.lum>\n";
        return 0;
    }

    std::string command = argv[1];

    if (command == "tokenize") {
        if (argc < 3) {
            std::cerr << "Error: Missing filename.\n";
            return 1;
        }

        std::ifstream file(argv[2]);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file " << argv[2] << "\n";
            return 1;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();

        Lexer lexer(source);
        try {
            auto tokens = lexer.tokenize();
            printTokens(tokens);
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
