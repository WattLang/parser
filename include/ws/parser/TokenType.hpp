#pragma once

#include <iostream>

namespace ws::parser {

enum class TokenType {
    Parenthesis,
    Operator,
    Literal

};

inline std::ostream& operator<<(std::ostream& os, TokenType type) {
    switch(type) {
        case TokenType::Parenthesis: return os << "parenthesis";
        case TokenType::Operator: return os << "operator";
        case TokenType::Literal: return os << "literal";
        default: return os << "unknown";
    }
}

enum class TokenSubType {
    Left, Right,
    Plus, Minus, Multiplication, Division,
    Float

};

inline std::ostream& operator<<(std::ostream& os, TokenSubType subtype) {
    switch(subtype) {
        case TokenSubType::Left: return os << "left";
        case TokenSubType::Right: return os << "right";
        case TokenSubType::Plus: return os << "plus";
        case TokenSubType::Minus: return os << "minus";
        case TokenSubType::Multiplication: return os << "multiplication";
        case TokenSubType::Division: return os << "division";
        case TokenSubType::Float: return os << "float";
        default: return os << "unknown";
    }
}

}