#include <iostream>
#include <vector>
#include <iomanip>

#include <module/module.h>
#include <ws/parser/Parser.hpp>
#include <ws/parser/token/Token.hpp>

ws::parser::Token number(float f) {
    return {std::to_string(f), ws::parser::TokenType::Literal, ws::parser::TokenSubType::Float};
}

ws::parser::Token plus() {
    return {"+", ws::parser::TokenType::Operator, ws::parser::TokenSubType::Plus};
}

ws::parser::Token mult() {
    return {"*", ws::parser::TokenType::Operator, ws::parser::TokenSubType::Multiplication};
}

ws::parser::Token div() {
    return {"/", ws::parser::TokenType::Operator, ws::parser::TokenSubType::Division};
}

ws::parser::Token sub() {
    return {"-", ws::parser::TokenType::Operator, ws::parser::TokenSubType::Minus};
}

ws::parser::Token left() {
    return {"(", ws::parser::TokenType::Parenthesis, ws::parser::TokenSubType::Left};
}

ws::parser::Token right() {
    return {")", ws::parser::TokenType::Parenthesis, ws::parser::TokenSubType::Right};
}

#define CHECK(parsable, print_ast, is...) check({ is }, parsable, print_ast)
#define P plus()
#define S sub()
#define M mult()
#define D div()
#define L left()
#define R right()
#define N(n) number(n)

bool check(std::vector<ws::parser::Token> const& tokens, bool parsable, bool print_ast) {
    auto out = ws::parser::parse(tokens);

    ws::module::print("Expression【", std::fixed, std::setprecision(2));
    bool is_first_token = true;
    for(auto const& t : tokens) {
        if (!is_first_token)
            ws::module::print(" ");
        try {
             ws::module::print(std::stof(t.content));
        } catch(...) {
            ws::module::print(t.content);
        }
        is_first_token = false;
    }
    ws::module::println("】...");

    bool test_pass = !is_error(out) == parsable;

    if (test_pass)
        ws::module::print(ws::module::colour::fg::green, ws::module::style::bold, "[_]", ws::module::style::reset, " OK");
    else
        ws::module::error("ERROR");

    if (print_ast && !is_error(out))
        ws::module::println(": ", (*get_ast(out))->compile().dump(2));
    else if (is_error(out))
        ws::module::println(": ", get_error(out)->what());
    else
        ws::module::println();

    ws::module::println();
    return test_pass;
}

int main(int argc, char** argv) {
    bool print_ast = argc > 1 && std::string(argv[1]) == "--ast";

#define CHECK_T(is...) CHECK(true, print_ast, is)
#define CHECK_F(is...) CHECK(false, print_ast, is)

    bool all_test =
       CHECK_F()
    && CHECK_T( N(42))
    && CHECK_T( L, N(1), R)
    && CHECK_F( L, N(1))
    && CHECK_F( N(1), R)
    && CHECK_F( L, R)
    && CHECK_T( L, L, L, N(1), R, R, R)
    && CHECK_T( S, N(42))
    && CHECK_T( L, S, N(1), R)
    && CHECK_T( S, L, N(1), R)
    && CHECK_F( S)
    && CHECK_T( S, S, S, N(42))
    && CHECK_F( N(42), N(1337))
    && CHECK_F( N(42), N(1337), N(666))
    && CHECK_T( N(42), M, N(666))
    && CHECK_F( N(42), M, N(666), S)
    && CHECK_T( N(42), M, S, N(666))
    && CHECK_T( N(42), M, S, S, N(666))
    && CHECK_T( S, N(42), M, N(666))
    && CHECK_T( S, S, N(42), M, N(666))
    && CHECK_T( N(42), S, N(666))
    && CHECK_T( N(42), D, N(666))
    && CHECK_T( N(42), P, N(1337))
    && CHECK_T( N(42), P, N(1337), D, N(1337))
    && CHECK_T( N(42), D, N(1337), P, N(1337))
    && CHECK_T( N(1337), P, N(42), D, N(1337), P, N(1337))
    && CHECK_T( N(42), D, N(1337), P, N(1337), D, N(1337))
    && CHECK_F( P, N(42), P, N(1337))
    && CHECK_F( N(42), P, D, N(1337))
    && CHECK_T( N(42), P, N(1337.5), P, N(666), P, S, S, N(1),  D, S, S, N(11));

    if (all_test)
        ws::module::println(ws::module::colour::fg::green, ws::module::style::bold, "[_]", ws::module::style::reset, " Pass all tests");
    else
        ws::module::errorln("Error on tests");

    return all_test ? 0 : 1;
}