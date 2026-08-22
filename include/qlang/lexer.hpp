#pragma once

#include "qlang/token.hpp"
#include <string_view>
#include <vector>

namespace qlang { //* merges with the one in token.hpp

class Lexer {
public:
    explicit Lexer(std::string_view src) 
        : src_{src} {}
    
        std::vector<Token> tokenize();

private:
    Token next();
    void skip_whitespace();

    char peek(size_t offset = 0) const { //* read only
        size_t j = i_ + offset;
        return j < src_.size() ? src_[j] : '\0'; //* bound check
    }

    Token lex_identifier();
    Token lex_number();
    Token lex_string();

    std::string_view src_;
    size_t i_ = 0;  //* current read position

};

}