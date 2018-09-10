#pragma once

#include <string>

#include <ws/parser/token/TokenType.hpp>

namespace ws::parser {

class Token {
public:

    Token(std::string const& content, TokenType type, TokenSubType subtype);

    std::string content;
    TokenType type;
    TokenSubType subtype;

};

std::ostream& operator<<(std::ostream& os, Token const& token);

}