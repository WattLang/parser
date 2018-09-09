#pragma once

#include <string>

#include <ws/parser/TokenType.hpp>

namespace ws::parser {

class Token {
public:

    Token(std::string const& content, TokenType type, TokenSubType subtype) : content(content), type(type), subtype(subtype) {}

    std::string content;
    TokenType type;
    TokenSubType subtype;

};

inline std::ostream& operator<<(std::ostream& os, Token const& token) {
    return os << "{" << token.content << " : " << token.type << "." << token.subtype << "}";
}

}