#include <catch2/catch_test_macros.hpp>
#include <string_view>
#include "qlang/parser.hpp"
#include "qlang/lexer.hpp"
#include "qlang/ast_print.hpp"

using namespace qlang;

namespace {
    std::string print_query(std::string_view source) {
        auto toks = Lexer(source).tokenize();
        return print(*Parser(std::move(toks), source).parse());
    }
}


TEST_CASE("AND binds tighter than OR") {
    REQUIRE(print_query
        ("a:1 OR b:2 AND c:3") == "(OR a:1 (AND b:2 c:3))");
}

TEST_CASE("AND is left associative") {
    REQUIRE(print_query
        ("a:1 AND b:2 AND c:3") == "(AND (AND a:1 b:2) c:3)");
}

TEST_CASE("parentheses override precedence") {
    REQUIRE(print_query("(a:1 OR b:2) AND c:3") == "(AND (OR a:1 b:2) c:3)");
}