#pragma once

#include <string>

#include <ws/parser/TokenType.hpp>

namespace ws { namespace parser {

class Token {
public:

    Token(std::string const& content, TokenType type, TokenSubType subtype) : content(content), type(type), subtype(subtype) {}

    std::string content;
    TokenType type;
    TokenSubType subtype;

};

}}