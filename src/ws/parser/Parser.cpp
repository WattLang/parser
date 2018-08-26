#include <ws/parser/Parser.hpp>

#include <ws/parser/AST.hpp>
#include <ws/parser/Number.hpp>
#include <ws/parser/BinaryOperator.hpp>

#include <iostream>

namespace ws { namespace parser {

parse_result parse_number(std::size_t& index, std::vector<Token> const& tokens);
parse_result parse_term(std::size_t& index, std::vector<Token> const& tokens);
parse_result parse_expression(std::size_t& index, std::vector<Token> const& tokens);
parse_result parse_low_binary_operation(std::size_t& index, std::vector<Token> const& tokens);
parse_result parse_high_binary_operation(std::size_t& index, std::vector<Token> const& tokens);

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

bool is_error(parse_result const& res) {
    return !std::holds_alternative<std::unique_ptr<AST>>(res);
}

std::string get_message(parse_result const& res) {
    if (auto error = std::get_if<ParserError>(&res); error) 
        return error->what();
    return std::get<std::unique_ptr<AST>>(res)->compile(0);
}

parse_result parse(std::vector<Token> const& tokens) {
    std::size_t i = 0;

    try {
        auto expression = parse_expression(i, tokens);
        if (i < tokens.size())
            return ParserError::expected({"expression"});
    
        return expression;

    } catch(std::out_of_range const&) {
        return ParserError::error();
    }
}

parse_result parse_number(std::size_t& index, std::vector<Token> const& tokens) {
    auto& token = tokens.at(index++);

    if (token.type != TokenType::Litteral || token.subtype != TokenSubType::Float) 
        return ParserError::expected({"number litteral"});

    return std::make_unique<Number>(
        token.content
    );
}

parse_result parse_term(std::size_t& index, std::vector<Token> const& tokens) {
    auto& token = tokens.at(index);

    switch(token.type) {
        case TokenType::Litteral: 
            if (token.subtype == TokenSubType::Float)
                return parse_number(index, tokens);
            else 
                return ParserError::expected({"number litteral"});
        default:
            return ParserError::expected({"litteral"});
    }
}

parse_result parse_expression(std::size_t& index, std::vector<Token> const& tokens) {
    return parse_low_binary_operation(index, tokens);
}

parse_result parse_low_binary_operation(std::size_t& index, std::vector<Token> const& tokens) {
    auto lhs = parse_high_binary_operation(index, tokens);
    if (is_error(lhs))
        return lhs;

    if (index >= tokens.size())
        return lhs;

    auto* token = &tokens.at(index);

    while(token->type == TokenType::Operator && (token->subtype == TokenSubType::Plus || token->subtype == TokenSubType::Minus)) {
        index++;
        std::string name;
        switch(token->subtype) {
            case TokenSubType::Plus:
                name = "plus"; break;
            case TokenSubType::Minus:
                name = "subtract"; break;
            default:
                return ParserError::unknown_token(token->content);
        }

        auto rhs = parse_high_binary_operation(index, tokens);

        if (is_error(rhs))
            return rhs;

        lhs = std::make_unique<BinaryOperator>(
            name,
            std::move(std::get<std::unique_ptr<AST>>(lhs)),
            std::move(std::get<std::unique_ptr<AST>>(rhs))
        );

        if (index >= tokens.size())
            return lhs;
            
        token = &tokens.at(index);
    }

    return lhs;
}

parse_result parse_high_binary_operation(std::size_t& index, std::vector<Token> const& tokens) {
    auto lhs = parse_term(index, tokens);

    if (is_error(lhs))
        return lhs;

    if (index >= tokens.size())
        return lhs;

    auto* token = &tokens.at(index);

    while(token->type == TokenType::Operator && (token->subtype == TokenSubType::Division || token->subtype == TokenSubType::Multiplication)) {
        index++;
        std::string name;
        switch(token->subtype) {
            case TokenSubType::Division:
                name = "division"; break;
            case TokenSubType::Multiplication:
                name = "multiplication"; break;
            default:
                return ParserError::unknown_token(token->content);
        }

        auto rhs = parse_term(index, tokens);

        if (is_error(rhs))
            return rhs;

        lhs = std::make_unique<BinaryOperator>(
            name,
            std::move(std::get<std::unique_ptr<AST>>(lhs)),
            std::move(std::get<std::unique_ptr<AST>>(rhs))
        );

        if (index >= tokens.size())
            return lhs;
            
        token = &tokens.at(index);
    }

    return lhs;
}

}}