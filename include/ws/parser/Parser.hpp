#pragma once

#include <vector>
#include <string>
#include <variant>
#include <memory>
#include <optional>

#include <ws/parser/Token.hpp>
#include <ws/parser/AST.hpp>

namespace ws { namespace parser {

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

using parse_result = std::variant<std::unique_ptr<AST>, ParserError>;

bool is_error(parse_result const& res);
std::string get_message(parse_result const& res);

parse_result parse(std::vector<Token> const& tokens);

}}