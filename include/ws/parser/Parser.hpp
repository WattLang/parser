#pragma once

#include <vector>
#include <string>

#include <optional>

#include <ws/parser/Token.hpp>

namespace ws { namespace parser {

std::optional<std::string> parse(std::vector<Token> const& tokens);

}}