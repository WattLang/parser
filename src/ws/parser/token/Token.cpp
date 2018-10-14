#include <ws/parser/token/Token.hpp>

namespace ws::parser {

Token::Token(std::string const& content, TokenType type, TokenSubType subtype, std::size_t line, std::size_t column) 
    : content(content), type(type), subtype(subtype), line(line), column(column) {}

std::ostream& operator<<(std::ostream& os, Token const& token) {
    return os << "{" << token.content << " : " << token.type << "." << token.subtype << " at " << token.line << ":" << token.column << "}";
}

}