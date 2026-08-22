#include "qlang/lexer.hpp"
#include "qlang/token.hpp"
#include <cstddef>

namespace qlang {

namespace { //* anonymous namespace: helpers are private to this file

//! The rule that follows: anything in a .cpp that isn't declared in the corresponding header should be in an anonymous namespace. If it's not part of the public interface, it shouldn't be a visible symbol.

bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; //* _interval is a valid name
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

TokenType keyword_or_identifier(std::string_view text) {
    if (text == "AND" || text == "and") return TokenType::And;
    if (text == "OR" || text == "or") return TokenType::Or;
    if (text == "NOT" || text == "not") return TokenType::Not;
    return TokenType::Identifier;
}

}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens{};
    while (true) {
        Token cur{next()};
        tokens.push_back(cur);
        if (cur.type == TokenType::End) return tokens;      //* Include the End token
    }
}

Token Lexer::next() {
    //* Skip trailing whitespaces 
    skip_whitespace();
    
    //* End of the query
    if (i_ >= src_.size()) {
        return Token{TokenType::End, {}, i_};
    }
    
    size_t start = i_; //* Begin index

    //* Op cases
    switch (peek()) {
        case '~': {
            i_ += 1;
            return Token{TokenType::Tilde, {src_.substr(start, i_ - start)}, start};
        }
        case '(': {
            i_ += 1;
            return Token{TokenType::LParen, {src_.substr(start, i_ - start)}, start};
        }
        case ')': {
            i_ += 1;
            return Token{TokenType::RParen, {src_.substr(start, i_ - start)}, start};
        }
        case ':': {
            switch (peek(1)) {
            case '!': {

                i_ += 2;
                return Token{TokenType::Bang,{src_.substr(start, i_ - start)}, start};
                }
            case '<': {

                if (peek(2) == '=') {
                    i_ += 3;
                    return Token{TokenType::LtEq, src_.substr(start, i_ - start), start};
                }
                i_ += 2;
                return Token{TokenType::Lt, src_.substr(start, i_ - start), start};
                }
            case '>': {

                if (peek(2) == '=') {
                    i_ += 3;
                    return Token{TokenType::GtEq, src_.substr(start, i_ - start), start};
                }
                i_ += 2;
                return Token{TokenType::Gt, src_.substr(start, i_ - start), start};
                } 
            default: {

                i_ += 1;
                return Token{TokenType::Colon, {src_.substr(start, i_ - start)}, start};
                }
            }
        }
    }        
    
    if (peek() == '"') return lex_string();
    if (is_alpha(peek())) return lex_identifier();
    if (is_digit(peek())) return lex_number();
    
    i_++;
    
    return Token{TokenType::Invalid, {src_.substr(start, i_ - start)}, start};
}

void Lexer::skip_whitespace() {
    while (is_whitespace(peek())) ++i_;
}

// * Consumes IDENT
Token Lexer::lex_identifier() {
    size_t start = i_;
    while (is_alpha(peek()) || is_digit(peek())) {
        i_++;
    }
    std::string_view text{src_.substr(start, i_ - start)};
    return Token{keyword_or_identifier(text), {text}, start};
}

//* Consumes number
Token Lexer::lex_number() {
    size_t start = i_;
    bool seen_dot = false;

    while (is_digit(peek()) || (peek() == '.' && !seen_dot)) {
        if (peek() == '.') seen_dot = true;
        ++i_;
    }
    return Token{TokenType::Number, {src_.substr(start, i_ - start)}, start};
}

//* Consumes string (with double quotes)
Token Lexer::lex_string() {
    size_t start = i_;
    i_++;
    while (peek() != '"' && peek() != '\0') { //* Stop at next doublew quote
        i_++;
    }

    if (peek() == '\0') { //* Prevents invalid string
        return Token{TokenType::Invalid, src_.substr(start, i_ - start), start};
    }

    i_++; //* Count the trailing double quote 
    return Token{TokenType::String, {src_.substr(start, i_ - start)}, start};
}

} //* namespace

