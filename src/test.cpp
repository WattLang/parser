#include <iostream>
#include <vector>
#include <iomanip>

#include <module/module.h>
#include <ws/parser/Parser.hpp>
#include <ws/parser/Token.hpp>

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
        ws::module::print(ws::module::colour::fg::green, ws::module::style::bold, "[O]", ws::module::style::reset, " OK");
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

    bool all_test =
        check({}, false, print_ast)
    &&  check({number(42)}, true, print_ast)
    &&  check({sub(), number(42)}, true, print_ast)
    &&  check({sub()}, false, print_ast)
    &&  check({sub(), sub(), sub(), number(42)}, true, print_ast)
    &&  check({number(42), number(1337)}, false, print_ast)
    &&  check({number(42), number(1337), number(666)}, false, print_ast)
    &&  check({number(42), mult(), number(666)}, true, print_ast)
    &&  check({number(42), mult(), number(666), sub()}, false, print_ast)
    &&  check({number(42), mult(), sub(), number(666)}, true, print_ast)
    &&  check({number(42), mult(), sub(), sub(), number(666)}, true, print_ast)
    &&  check({sub(), number(42), mult(), number(666)}, true, print_ast)
    &&  check({sub(), sub(), number(42), mult(), number(666)}, true, print_ast)
    &&  check({number(42), sub(), number(666)}, true, print_ast)
    &&  check({number(42), div(), number(666)}, true, print_ast)
    &&  check({number(42), plus(), number(1337)}, true, print_ast)
    &&  check({number(42), plus(), number(1337), div(), number(1337)}, true, print_ast)
    &&  check({number(42), div(), number(1337), plus(), number(1337)}, true, print_ast)
    &&  check({number(1337), plus(), number(42), div(), number(1337), plus(), number(1337)}, true, print_ast)
    &&  check({number(42), div(), number(1337), plus(), number(1337), div(), number(1337)}, true, print_ast)
    &&  check({plus(), number(42), plus(), number(1337)}, false, print_ast)
    &&  check({number(42), plus(), div(), number(1337)}, false, print_ast);

    if (all_test)
        ws::module::println(ws::module::colour::fg::green, ws::module::style::bold, "[O]", ws::module::style::reset, " Pass all tests");
    else
        ws::module::errorln("Error on tests");

    return all_test ? 0 : 1;
}