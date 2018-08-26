#include <ws/parser/Parser.hpp>

#include <ws/parser/AST.hpp>
#include <ws/parser/Number.hpp>
#include <ws/parser/BinaryOperator.hpp>

#include <iostream>

namespace ws { namespace parser {

std::unique_ptr<AST> parse_number(std::size_t& index, std::vector<Token> const& tokens);
std::unique_ptr<AST> parse_term(std::size_t& index, std::vector<Token> const& tokens);
std::unique_ptr<AST> parse_expression(std::size_t& index, std::vector<Token> const& tokens);
std::unique_ptr<AST> parse_low_binary_operation(std::size_t& index, std::vector<Token> const& tokens);
std::unique_ptr<AST> parse_high_binary_operation(std::size_t& index, std::vector<Token> const& tokens);

std::optional<std::string> parse(std::vector<Token> const& tokens) {
    std::size_t i = 0;

    try {
        auto expression = parse_expression(i, tokens);
        if (!expression || i < tokens.size())
            return {};
    
        return expression->compile(0);

    } catch(std::out_of_range const&) {
        return {};
    }
}

std::unique_ptr<AST> parse_number(std::size_t& index, std::vector<Token> const& tokens) {
    auto& token = tokens.at(index++);

    if (token.type != TokenType::Litteral || token.subtype != TokenSubType::Float) 
        return nullptr;

    return std::make_unique<Number>(
        token.content
    );
}

std::unique_ptr<AST> parse_term(std::size_t& index, std::vector<Token> const& tokens) {
    auto& token = tokens.at(index);

    switch(token.type) {
        case TokenType::Litteral: 
            if (token.subtype == TokenSubType::Float)
                return parse_number(index, tokens);
            else 
                return nullptr;
        default:
            return nullptr;
    }
}

std::unique_ptr<AST> parse_expression(std::size_t& index, std::vector<Token> const& tokens) {
    return parse_low_binary_operation(index, tokens);
}

std::unique_ptr<AST> parse_low_binary_operation(std::size_t& index, std::vector<Token> const& tokens) {
    std::unique_ptr<AST> lhs = parse_high_binary_operation(index, tokens);

    if (index >= tokens.size())
        return std::move(lhs);

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
                return nullptr;
        }

        auto rhs = parse_high_binary_operation(index, tokens);

        if (!rhs || !lhs)
            return nullptr;

        lhs = std::make_unique<BinaryOperator>(
            name,
            std::move(lhs),
            std::move(rhs)
        );

        if (index >= tokens.size())
            return std::move(lhs);
            
        token = &tokens.at(index);
    }

    return std::move(lhs);
}

std::unique_ptr<AST> parse_high_binary_operation(std::size_t& index, std::vector<Token> const& tokens) {
    std::unique_ptr<AST> lhs = parse_term(index, tokens);

    if (index >= tokens.size())
        return std::move(lhs);

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
                return nullptr;
        }

        auto rhs = parse_term(index, tokens);

        if (!rhs || !lhs)
            return nullptr;

        lhs = std::make_unique<BinaryOperator>(
            name,
            std::move(lhs),
            std::move(rhs)
        );

        if (index >= tokens.size())
            return std::move(lhs);
            
        token = &tokens.at(index);
    }

    return std::move(lhs);
}

}}