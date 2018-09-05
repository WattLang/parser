#include <iostream>
#include <vector>
#include <iomanip>

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

    std::cout << " `" << std::fixed << std::setprecision(2);
    for(auto const& t : tokens) {
        try {
            float f = std::stof(t.content);
            std::cout << f << " ";
        } catch(...) {
            std::cout << t.content << " ";
        }
    }
    std::cout << "`\n\t";

    bool test_pass = !is_error(out) == parsable;

    if (test_pass)
        std::cout << "OK";
    else 
        std::cout << "ERROR";

    if (print_ast && !is_error(out))
        std::cout << ": " << (*get_ast(out))->compile(4) << '\n';
    else if (is_error(out))
        std::cout << ": " << get_error(out)->what() << '\n';
    else 
        std::cout << '\n';

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

    return all_test ? 0 : 1;
}