// tests/test_typecheck.cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "qlang/lexer.hpp"
#include "qlang/parser.hpp"
#include "qlang/schema.hpp"
#include "qlang/typecheck.hpp"

#include <string_view>
#include <vector>

using namespace qlang;
using Catch::Matchers::ContainsSubstring;

namespace {

// Runs the whole front half of the pipeline: text -> tokens -> tree -> errors.
// Safe to return: TypeError owns its message, and nothing returned points into `query`.
std::vector<TypeError> check(std::string_view query, const Schema& schema) {
    Lexer lexer{query};
    Parser parser{lexer.tokenize(), query};
    NodePtr ast = parser.parse();
    return typecheck(*ast, schema);
}

}  // namespace

TEST_CASE("unknown field suggests every equally close name, alphabetically", "[typecheck]") {
    Schema schema;
    REQUIRE(schema.add("dog", ColumnType::Int));   // 3 edits from "cax": past the limit
    REQUIRE(schema.add("cat", ColumnType::Int));   // 1 edit: x -> t
    REQUIRE(schema.add("car", ColumnType::Int));   // 1 edit: x -> r

    const auto errors = check("cax:1", schema);

    REQUIRE(errors.size() == 1);
    REQUIRE_THAT(errors[0].message, ContainsSubstring("did you mean 'car' or 'cat'?"));
}

TEST_CASE("operator ':>' requires 'int', but field 'name' is declared 'string' ") {
    Schema schema;
    REQUIRE(schema.add("name", ColumnType::String));
    REQUIRE(schema.add("date", ColumnType::Int));
    REQUIRE(schema.add("came", ColumnType::String));

    const auto errors = check("name:>5", schema);

    REQUIRE(errors.size() == 1);
    REQUIRE_THAT(errors[0].message, ContainsSubstring("operator ':>'"));
    REQUIRE_THAT(errors[0].message, ContainsSubstring("field 'name'"));
}
