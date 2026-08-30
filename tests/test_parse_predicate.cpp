#include <catch2/catch_test_macros.hpp>
#include "qlang/lexer.hpp"
#include "qlang/parser.hpp"

using namespace qlang;

TEST_CASE("Testing predicate parsing valid token") {
    std::string src{"level:error"};
    auto toks = Lexer{src}.tokenize();
    Parser p{std::move(toks), src};
    NodePtr n = p.parse();

    auto* pred = dynamic_cast<Predicate*>(n.get());
    REQUIRE(pred != nullptr);
    REQUIRE(pred->field == "level");
    REQUIRE(pred->op == CmpOp::Eq);
}