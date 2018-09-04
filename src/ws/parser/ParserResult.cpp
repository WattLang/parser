#include <ws/parser/ParserResult.hpp>

namespace ws::parser {

ParserError ParserError::expected(std::vector<std::string> const& tokens) {
    std::string error = "Excepted one of ";
    for(std::size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0)
            error += ", ";
        error += '`' + tokens[i] + "`";
    }
    return { error };
}
ParserError ParserError::unknown_token(std::string const& token) {
    return {"Unknown token `" + token + "`"};
}

ParserError ParserError::error() {
    return {"Unknown error"};
}

std::string ParserError::what() const {
    return error_msg;
}

ParserError::ParserError(std::string const& error_msg) : error_msg(error_msg) {}

bool is_error(ParserResult const& res) {
    return get_error(res) != nullptr;
}

std::string get_message(ParserResult const& res) {
    if (auto error = get_error(res); error)
        return error->what();
    return (*get_ast(res))->compile(0);
}

AST_ptr* get_ast(ParserResult& res) {
    return std::get_if<AST_ptr>(&res);
}

ParserError* get_error(ParserResult& res) {
    return std::get_if<ParserError>(&res);
}

AST_ptr const*  get_ast(ParserResult const& res) {
    return std::get_if<AST_ptr>(&res);
}

ParserError const* get_error(ParserResult const& res) {
    return std::get_if<ParserError>(&res);
}

}