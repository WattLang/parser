#pragma once

#include <string>

#include <ws/parser/TokenType.hpp>

namespace ws { namespace parser {

class Token {
public:

    std::string content;
    TokenType type;
    TokenSubType subtype;

};

}}