#include <ws/parser/token/Token.hpp>

namespace ws::parser {

Token::Token(std::string const& content, TokenType type, TokenSubType subtype) : content(content), type(type), subtype(subtype) {}

std::ostream& operator<<(std::ostream& os, Token const& token) {
    return os << "{" << token.content << " : " << token.type << "." << token.subtype << "}";
}

}