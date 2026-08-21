#include <iostream>
#include <string>
#include "qlang/lexer.hpp"

int main(int argc, char* argv[]) {
    //* argc = number of arguments (including the program name)
    //* argv[0] = program name, argv[1] = first real argument

    if (argc == 3 && std::string(argv[1]) == "--tokens") {
        std::string source = argv[2];
        qlang::Lexer lexer{source};
        for (const auto& token : lexer.tokenize()) {
            std::cout << to_string(token.type) << " '" << token.text << "' @" << token.pos << "\n";
        } 
        return 0;
    }
    std::cerr << "usage: qlang --tokens '<query>'\n";
    return 1;
}