#include <iostream>
#include <vector>
#include <iomanip>

#include <ws/parser/Parser.hpp>
#include <ws/parser/Token.hpp>

ws::parser::Token number(float f) {
    return {std::to_string(f), ws::parser::TokenType::Litteral, ws::parser::TokenSubType::Float};
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

    if (out.has_value() == parsable) {
        std::cout << "OK!\n";
        if (print_ast && out)
            std::cout << *out << '\n';
        return true;
    } else {
        std::cout << "ERROR!\n";
        if (print_ast && out)
            std::cout << *out << '\n';
        return false;
    }
}

int main(int argc, char** argv) {
    bool print_ast = argc > 1 && std::string(argv[1]) == "--ast";

    bool all_test = 
        check({}, false, print_ast)
    &&  check({number(42)}, true, print_ast)
    &&  check({sub()}, false, print_ast)
    &&  check({number(42), number(1337)}, false, print_ast)
    &&  check({number(42), number(1337), number(666)}, false, print_ast)
    &&  check({number(42), mult(), number(666)}, true, print_ast)
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