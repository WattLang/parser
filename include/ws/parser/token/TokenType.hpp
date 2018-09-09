#pragma once

#include <iostream>

namespace ws::parser {

enum class TokenType {
    Parenthesis,
    Operator,
    Literal

};

std::ostream& operator<<(std::ostream& os, TokenType type);

enum class TokenSubType {
    Left, Right,
    Plus, Minus, Multiplication, Division,
    Float

};

std::ostream& operator<<(std::ostream& os, TokenSubType subtype);

}