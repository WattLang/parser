#include <iostream>
#include <vector>
#include <iomanip>
#include <optional>
#include <cmath>
#include <random>

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

std::optional<ws::parser::Token> tokenize(char c) {
    if (c >='0' && c <= '9')
        return number(static_cast<float>(c - '0'));

    static std::random_device rd;
    static std::mt19937 rng(rd());
    static std::uniform_real_distribution<float> uni(-99.99f, 100.f);

    switch(c) {
        case 'i': {
            auto n = uni(rng);
            n = std::round(n * 100.f) / 100.f;
            return number(n);
        }

        case '*':
            return mult();
        case '/':
            return div();
        case '+':
            return plus();
        case '-':
            return sub();

        case ')':
            return right();
        case '(':
            return left();

        default:
            return std::nullopt;
    }    
}

std::vector<ws::parser::Token> tokenize(std::string const& expr) {
    std::vector<ws::parser::Token> tokens;
    for(auto c : expr) {
        auto token = tokenize(c);
        if(token)
            tokens.emplace_back(*token);
    }

    return tokens;
}

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
        ws::module::success("OK");
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

#define CHECK_T(is...) check(tokenize(is), true, print_ast)
#define CHECK_F(is...) check(tokenize(is), false, print_ast)

    bool all_test =
       CHECK_F("")
    && CHECK_T("i")
    && CHECK_T("(i)")
    && CHECK_F("(i")
    && CHECK_F("i)")
    && CHECK_F("()")
    && CHECK_T("(((i)))")
    && CHECK_T("-i")
    && CHECK_T("(-i)")
    && CHECK_T("-(i)")
    && CHECK_F("-")
    && CHECK_T("---i")
    && CHECK_F("ii")
    && CHECK_F("iii")
    && CHECK_T("i*i")
    && CHECK_T("i*-i")
    && CHECK_T("i*--i")
    && CHECK_T("-i*i")
    && CHECK_T("--i*i")
    && CHECK_T("i-i")
    && CHECK_T("i/i")
    && CHECK_T("i+i")
    && CHECK_T("i+i/i")
    && CHECK_T("i/i+i")
    && CHECK_T("i+i/i+i")
    && CHECK_T("i/i+i/i")
    && CHECK_F("+i+i")
    && CHECK_F("i+/i")
    && CHECK_T("i+i+i+--i*--i");

    if (all_test)
        ws::module::successln("Pass all tests");
    else
        ws::module::errorln("Error on tests");

    return all_test ? 0 : 1;
}