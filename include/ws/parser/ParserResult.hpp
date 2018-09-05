#pragma once

#include <vector>
#include <string>
#include <variant>
#include <optional>
#include <memory>

#include <ws/parser/AST.hpp>

namespace ws::parser {

class ParserError {
public:

    static ParserError expected(std::vector<std::string> const& tokens);
    static ParserError unknown_token(std::string const& token);
    static ParserError error();

    std::string what() const;

private:

    ParserError(std::string const& error_msg);

    std::string error_msg;

};

using AST_ptr = std::unique_ptr<AST>;
using ParserResult = std::variant<AST_ptr, ParserError>;

bool is_error(ParserResult const& res);
std::string get_message(ParserResult const& res);

ParserError const* get_error(ParserResult const& error);
AST_ptr const* get_ast(ParserResult const& error);
ParserError* get_error(ParserResult& error);
AST_ptr* get_ast(ParserResult& error);

}