#pragma once

#include <vector>

#include <ws/parser/Token.hpp>
#include <ws/parser/ParserResult.hpp>

namespace ws::parser {

ParserResult parse(std::vector<Token> const& tokens);

}