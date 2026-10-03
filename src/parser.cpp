#include "qlang/parser.hpp"
#include "qlang/ast.hpp"
#include "qlang/token.hpp"
#include <memory>
#include <charconv>
#include <optional>
#include <algorithm>

namespace qlang {

namespace {
    int tok_type_to_bp(TokenType type) {
        switch (type) {
            case TokenType::Or: {return 1;}
            case TokenType::And: {return 3;}
            default:
                return -1;
        }
        return -1; //! Unknown token
    }

    bool is_value(TokenType type) {
        return (type == TokenType::Number || type == TokenType::String || type == TokenType::Identifier);
    }

    std::optional<BoolOp> try_bool_op(TokenType type) {
        if (type == TokenType::And) {
            return BoolOp::And;
        }
        if (type == TokenType::Or) {
            return BoolOp::Or;
        }
        return std::nullopt;
    }

    std::optional<CmpOp> try_cmp_op(TokenType type) {
        switch (type) {
            case TokenType::Colon: {return CmpOp::Eq;}
            case TokenType::Bang: {return CmpOp::NotEq;}
            case TokenType::Gt: {return CmpOp::Gt;}
            case TokenType::GtEq: {return CmpOp::GtEq;}
            case TokenType::Lt: {return CmpOp::Lt;}
            case TokenType::LtEq: {return CmpOp::LtEq;}
            case TokenType::Tilde: {return CmpOp::Contains;}
            case TokenType::Identifier:
            case TokenType::String:
            case TokenType::Number:
            case TokenType::Not:
            case TokenType::LParen:
            case TokenType::RParen:
            case TokenType::And:
            case TokenType::Or:
            case TokenType::End:
            case TokenType::Invalid:
                return std::nullopt;
        }
        return std::nullopt;
    }

    bool is_double(std::string_view num) {
        for (char c : num) {
            if (c == '.') {
                return true;
            }
        }
        return false;
    }

    Literal parse_literal(const Token& tok) {
        if (tok.type == TokenType::Number) {
            if (is_double(tok.text)) {
                double n{};
                std::from_chars(tok.text.data(), tok.text.data() + tok.text.size(), n);
                return n;
            } else {
                int64_t n{};
                std::from_chars(tok.text.data(), tok.text.data() + tok.text.size(), n);
                return n;
            }        
        } 
        if (tok.type == TokenType::Identifier) {
            if (tok.text == "true") return true;
            if (tok.text == "false") return false;
        }
        return std::string{tok.text};
    }
}
    
[[noreturn]] void Parser::error(const std::string& msg, size_t pos) const { 
    throw ParseError{msg, pos};    
}

std::string format_error(const ParseError& e, std::string_view source) {
    //* search backwards for '\n', starting at e.pos, towards 0
    size_t nl = source.rfind('\n', e.pos);
    size_t line_start = (nl == std::string_view::npos) ? 0 : nl + 1;
    nl = source.find('\n', e.pos);
    size_t line_end = (nl == std::string_view::npos) ? source.size() : nl;
    size_t offset = e.pos - line_start;
    size_t line = std::count(source.begin(), source.begin() + e.pos, '\n') + 1;
    std::string out;
    out += e.what();
    out += "\n";
    out += "  |\n";
    out += std::to_string(line);
    out += " |";
    out += source.substr(line_start, line_end - line_start);
    out += "\n";
    out += "  |";
    out += std::string(offset, ' ');
    out += "^";
    return out;
}   

bool Parser::expect(TokenType t) {
    if (peek().type != t) {return false;}
    advance();
    return true;
}

NodePtr Parser::parse() {
    auto expr = parse_expr(0);
    if (peek().type != TokenType::End) {
        error("error: expected an End token", peek().pos);
    }
    return expr;
}


NodePtr Parser::parse_expr(int min_bp) {
    auto lhs = parse_primary();    
    while(true) {
        int bp = tok_type_to_bp(peek().type);
        if (bp < min_bp) {break;}
        int next_min = bp + 1;
        
        Token cur = advance(); //! Must advance for a valid subsequent parse_expr call 
        std::optional<BoolOp> op = try_bool_op(cur.type);
        if (!op) {
            error("error: unknown operator", cur.pos);
        } 
        auto rhs = parse_expr(next_min);
        auto bi_ex = std::make_unique<BinaryExpr>();
        bi_ex->op = *op;
        bi_ex->lhs = std::move(lhs);
        bi_ex->rhs = std::move(rhs);
        lhs = std::move(bi_ex);
    }
    return lhs;
}

NodePtr Parser::parse_primary() {
    if (peek().type == TokenType::Not) {
        advance();
        NodePtr lhs = parse_primary();
        auto re_neg = std::make_unique<NotExpr>();
        re_neg->operand = std::move(lhs);
        return re_neg;
    }
    if (peek().type == TokenType::LParen) {
        advance();
        NodePtr expr = parse_expr(0); //! donot return to check primary grammar
        if (!expect(TokenType::RParen)) {
            error("error: unclosed primary, expected \")\"", peek().pos);
        }
        return expr;
    }
    return parse_predicate();
}

NodePtr Parser::parse_predicate() {
    if (peek().type != TokenType::Identifier) {
        error("error: expected a field", peek().pos);
    }
    Token field = advance();
    auto op = try_cmp_op(peek().type); 
    if (!op) {
        error("error: expected an operator", peek().pos);
    }
    advance();
    if (!is_value(peek().type)) {
        error("error: expected a literal", peek().pos);
    }
    Token value = advance();
    auto re = std::make_unique<Predicate>();
    re->field = field.text;
    re->op = *op;
    re->value = parse_literal(value);
    return re;
}


} //* namespace 
