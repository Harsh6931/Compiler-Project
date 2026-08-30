#include <iostream>

// My entry point for the compiler, it call Lexer,parser and parses CLI arguments.
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Lumen — toy language compiler\n"
                  << "Usage: lumen <command> [file.lum]\n"
                  << "Commands will be added in later stages "
                     "(tokenize, parse, run, ...).\n";
        return 0;
    }

    std::cout << "Command '" << argv[1]
              << "' is not implemented yet. Stage 0 scaffold only.\n";
    return 0;
}
