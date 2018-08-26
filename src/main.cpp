#include <iostream>
#include <vector>

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

bool check(std::vector<ws::parser::Token> const& tokens, bool parsable) {
    auto out = ws::parser::parse(tokens);

    if (out.has_value() == parsable) {
        std::cout << "OK!\n";
        if (out)
            std::cout << *out << '\n';
        return true;
    } else {
        std::cout << "ERROR!\n";
        if (out)
            std::cout << *out << '\n';
        return false;
    }
}

int main() {
    bool all_test = 
        check({}, false)
    &&  check({number(42)}, true)
    &&  check({sub()}, false)
    &&  check({number(42), number(1337)}, false)
    &&  check({number(42), number(1337), number(666)}, false)
    &&  check({number(42), mult(), number(666)}, true)
    &&  check({number(42), sub(), number(666)}, true)
    &&  check({number(42), div(), number(666)}, true)
    &&  check({number(42), plus(), number(1337)}, true)
    &&  check({number(42), plus(), number(1337), div(), number(1337)}, true)
    &&  check({number(42), div(), number(1337), plus(), number(1337)}, true)
    &&  check({number(1337), plus(), number(42), div(), number(1337), plus(), number(1337)}, true)
    &&  check({number(42), div(), number(1337), plus(), number(1337), div(), number(1337)}, true)
    &&  check({plus(), number(42), plus(), number(1337)}, false)
    &&  check({number(42), plus(), div(), number(1337)}, false);

    return all_test ? 0 : 1;
}