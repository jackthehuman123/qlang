#include <catch2/catch_test_macros.hpp>
#include "qlang/lexer.hpp"
#include "qlang/token.hpp"

//? How to test:
//? `cmake --build build -j`
//? `ctest --test-dir build --output-on-failure`

using namespace qlang;

TEST_CASE("empty input yields only End") {
    Lexer lex{""};
    auto tokens = lex.tokenize();
    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0].type == TokenType::End);
};

TEST_CASE("consume number and correct position") {
    std::string src = "level:error AND count:>=10";
    Lexer lex{src};
    auto tokens = lex.tokenize();

    REQUIRE(tokens[0].type == TokenType::Identifier);
    REQUIRE(tokens[0].text == "level");
    REQUIRE(tokens[0].pos == 0);

    REQUIRE(tokens[1].type == TokenType::Colon);
    REQUIRE(tokens[1].pos == 5);

    REQUIRE(tokens[2].type == TokenType::Identifier);
    REQUIRE(tokens[2].text == "error");
    REQUIRE(tokens[2].pos == 6);

    REQUIRE(tokens[3].type == TokenType::And);
    REQUIRE(tokens[3].text == "AND");
    REQUIRE(tokens[3].pos == 12);
    
    REQUIRE(tokens[4].type == TokenType::Identifier);
    REQUIRE(tokens[4].text == "count");
    REQUIRE(tokens[4].pos == 16);
    
    REQUIRE(tokens[5].type == TokenType::GtEq);
    REQUIRE(tokens[5].pos == 21);
    
    REQUIRE(tokens[6].type == TokenType::Number);
    REQUIRE(tokens[6].text == "10");
    REQUIRE(tokens[6].pos == 24);
    
}

TEST_CASE("whitespaces and others only source") {
    std::string src = "\n \t \r"; 
    Lexer lex{src};
    auto tokens = lex.tokenize();

    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0].type == TokenType::End);
}

TEST_CASE("tilde and not equal") {
    std::string src = R"(level:!error AND message~"timeout")";
    Lexer lex{src};
    auto tokens = lex.tokenize();

    REQUIRE(tokens[1].type == TokenType::Bang);
    REQUIRE(tokens[1].text == ":!");
    REQUIRE(tokens[1].pos == 5);

    REQUIRE(tokens[3].type == TokenType::And);
    REQUIRE(tokens[3].text == "AND");
    REQUIRE(tokens[3].pos == 13);    

    REQUIRE(tokens[5].type == TokenType::Tilde);
    REQUIRE(tokens[5].text == "~");
    REQUIRE(tokens[5].pos == 24);
}

TEST_CASE("two-character operator :>") {
    std::string src = "count:>10";
    Lexer lex{src};
    auto tokens = lex.tokenize();

    REQUIRE(tokens[1].type == TokenType::Gt);
    REQUIRE(tokens[1].text == ":>");
    REQUIRE(tokens[1].pos == 5);
}

TEST_CASE("three-character operator :>=") {
    std::string src = "count:>=10";
    Lexer lex{src};
    auto tokens = lex.tokenize();
    
    REQUIRE(tokens[1].type == TokenType::GtEq);
    REQUIRE(tokens[1].text == ":>=");
    REQUIRE(tokens[1].pos == 5);
}

TEST_CASE("two-character operator :<") {
    std::string src = "count:<10";
    Lexer lex{src};
    auto tokens = lex.tokenize();

    REQUIRE(tokens[1].type == TokenType::Lt);
    REQUIRE(tokens[1].text == ":<");
    REQUIRE(tokens[1].pos == 5);
}

TEST_CASE("three-character operator :<=") {
    std::string src = "count:<=10";
    Lexer lex{src};
    auto tokens = lex.tokenize();

    REQUIRE(tokens[1].type == TokenType::LtEq);
    REQUIRE(tokens[1].text == ":<=");
    REQUIRE(tokens[1].pos == 5);
}

TEST_CASE("bare colon is not swallowed by a longer operator") {
    std::string src = "level:error";
    Lexer lex{src};
    auto tokens = lex.tokenize();

    REQUIRE(tokens[1].type == TokenType::Colon);
    REQUIRE(tokens[1].text == ":");
    REQUIRE(tokens[1].pos == 5);
}

TEST_CASE("identifier starting with a keyword lexes as one identifier") {
    std::string src = "ANDROID:pixel";
    Lexer lex{src};
    auto tokens = lex.tokenize();

    REQUIRE(tokens[0].type == TokenType::Identifier);
    REQUIRE(tokens[0].text == "ANDROID");
    REQUIRE(tokens[0].pos == 0);

    REQUIRE(tokens[1].type == TokenType::Colon);
    REQUIRE(tokens[1].pos == 7);
}

TEST_CASE("quoted string containing a space") {
    std::string src = R"(message~"connection timeout")";
    Lexer lex{src};
    auto tokens = lex.tokenize();

    REQUIRE(tokens[0].type == TokenType::Identifier);
    REQUIRE(tokens[1].type == TokenType::Tilde);

    REQUIRE(tokens[2].type == TokenType::String);
    REQUIRE(tokens[2].text == R"("connection timeout")");
    REQUIRE(tokens[2].pos == 8);

    REQUIRE(tokens[3].type == TokenType::End);
}

TEST_CASE("unterminated string is Invalid") {
    std::string src = R"(message~"never closed)";
    Lexer lex{src};
    auto tokens = lex.tokenize();

    REQUIRE(tokens[2].type == TokenType::Invalid);
    REQUIRE(tokens[2].pos == 8);
    REQUIRE(tokens[3].type == TokenType::End);
}

TEST_CASE("integer and decimal numbers") {
    std::string src = "a:>10 AND b:<3.14";
    Lexer lex{src};
    auto tokens = lex.tokenize();

    REQUIRE(tokens[2].type == TokenType::Number);
    REQUIRE(tokens[2].text == "10");

    REQUIRE(tokens[6].type == TokenType::Number);
    REQUIRE(tokens[6].text == "3.14");
}

TEST_CASE("malformed number stops at the second dot") {
    std::string src = "a:>1.2.3";
    Lexer lex{src};
    auto tokens = lex.tokenize();

    REQUIRE(tokens[2].type == TokenType::Number);
    REQUIRE(tokens[2].text == "1.2");
    REQUIRE(tokens[2].pos == 3);

    REQUIRE(tokens[3].type == TokenType::Invalid);
    REQUIRE(tokens[3].text == ".");
    REQUIRE(tokens[3].pos == 6);

    REQUIRE(tokens[4].type == TokenType::Number);
    REQUIRE(tokens[4].text == "3");
}

TEST_CASE("nested parentheses") {
    std::string src = "((a:1))";
    Lexer lex{src};
    auto tokens = lex.tokenize();

    REQUIRE(tokens[0].type == TokenType::LParen);
    REQUIRE(tokens[0].pos == 0);
    REQUIRE(tokens[1].type == TokenType::LParen);
    REQUIRE(tokens[1].pos == 1);

    REQUIRE(tokens[5].type == TokenType::RParen);
    REQUIRE(tokens[6].type == TokenType::RParen);
    REQUIRE(tokens[7].type == TokenType::End);
}

TEST_CASE("garbage character is Invalid and lexing continues") {
    std::string src = "a:1 @ b:2";
    Lexer lex{src};
    auto tokens = lex.tokenize();

    REQUIRE(tokens[3].type == TokenType::Invalid);
    REQUIRE(tokens[3].text == "@");
    REQUIRE(tokens[3].pos == 4);

    REQUIRE(tokens[4].type == TokenType::Identifier);
    REQUIRE(tokens[4].text == "b");
}
