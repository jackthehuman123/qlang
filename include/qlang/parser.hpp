#pragma once 

#include "qlang/ast.hpp"
#include "qlang/token.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include <string_view>
#include <cstddef>

namespace qlang {

struct ParseError : std::runtime_error {
    size_t pos;
    ParseError(const std::string& msg, size_t p)
        : std::runtime_error(msg), pos{p} {}
};

class Parser {
public:
    //! std::vector<Token>              | const std::vector<Token>&
    //! copy / move (saves allocation)  | copy (expensive anyways)
    Parser(std::vector<Token> tokens, std::string_view source)
        : tokens_{std::move(tokens)}, src_{source} {}

    NodePtr parse(); //! throws ParseError on bad input

private:
    NodePtr parse_expr(int min_bp);
    NodePtr parse_primary();
    NodePtr parse_predicate();

    const Token& peek() const {return tokens_[i_];}
    const Token& advance()    {return tokens_[i_++];}
    bool expect(TokenType t);
    [[noreturn]] void error(const std::string& msg, size_t pos) const;

    std::vector<Token> tokens_;
    std::string_view   src_;
    size_t             i_ = 0;
};

//! Uses the thrown ParseError, so no access to the members needed 
//! -> Free function
std::string format_error(const ParseError& e, std::string_view source);

} //* namespace qlang