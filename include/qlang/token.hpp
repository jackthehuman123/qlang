#pragma once //* always include for headers
 
#include <string_view>
#include <cstddef>

namespace qlang {

enum class TokenType {
    Identifier,     //* level, count, error
    Number,         //* 10, 3.14
    String,         //* "timeout"
    
    Colon,          //* :
    Tilde,          //* ~
    Gt, GtEq,       //* :> :>=
    Lt, LtEq,       //* :< :<=
    Bang,           //* :!

    LParen, RParen, //* (, )
    And, Or, Not,   

    End,            //* end of input
    Invalid,        //* something we couldn't lex
};

struct Token {
    TokenType type;
    std::string_view text;  //* points into the source string
    size_t pos;             //* byte offset of the first character / lowk the index
};

inline std::string_view to_string(TokenType type) {
    switch (type) {
        case TokenType::Identifier: return "Identifier";
        case TokenType::Number:     return "Number";
        case TokenType::String:     return "String";
        case TokenType::Colon:      return "Colon";
        case TokenType::Tilde:      return "Tilde";
        case TokenType::Gt:         return "Gt";
        case TokenType::GtEq:       return "GtEq";
        case TokenType::Lt:         return "Lt";
        case TokenType::LtEq:       return "LtEq";
        case TokenType::Bang:       return "Bang";
        case TokenType::And:        return "And";
        case TokenType::Or:         return "Or";
        case TokenType::LParen:     return "LParen";
        case TokenType::RParen:     return "RParen";
        case TokenType::Not:        return "Not";   
        case TokenType::End:        return "End";
        case TokenType::Invalid:    return "Invalid";
        //! No default to prevent missing case
    }
    return "???";
}

}