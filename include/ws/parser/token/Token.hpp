#pragma once

#include <string>

#include <ws/parser/token/TokenType.hpp>

namespace ws::parser {

class Token {
public:

    Token() = default;
    Token(std::string const& content, TokenType type, TokenSubType subtype, std::size_t line, std::size_t column);

    std::string content;
    TokenType type;
    TokenSubType subtype;

    std::size_t line;
    std::size_t column;

};

std::ostream& operator<<(std::ostream& os, Token const& token);

}