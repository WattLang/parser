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
        std::cout << "parse # index: " << i << '\n';
        if (!expression || i < tokens.size())
            return std::cout << "parse = {}\n", std::nullopt;
    
        return expression->compile(0);

    } catch(std::out_of_range const&) {
        return std::cout << "parse (/!\\ out_of_range) = {}\n", std::nullopt;
    }
}

std::unique_ptr<AST> parse_number(std::size_t& index, std::vector<Token> const& tokens) {
    auto& token = tokens.at(index++);
    std::cout << "parse_number # index: " << index << '\n';

    if (token.type != TokenType::Litteral || token.subtype != TokenSubType::Float) 
        return std::cout << "parse_number = nullptr\n", nullptr;

    return std::cout << "parse_number = <number>\n", std::make_unique<Number>(
        token.content
    );
}

std::unique_ptr<AST> parse_term(std::size_t& index, std::vector<Token> const& tokens) {
    auto& token = tokens.at(index);
    std::cout << "parse_term # index: " << index << '\n';

    switch(token.type) {
        case TokenType::Litteral: 
            if (token.subtype == TokenSubType::Float)
                return std::cout << "parse_term = <number>\n", parse_number(index, tokens);
            else 
                return std::cout << "parse_term = nullptr\n", nullptr;
        default:
            return std::cout << "parse_term = nullptr\n", nullptr;
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
    std::cout << "parse_low_binary_operation # index: " << index << '\n';

    while(token->type == TokenType::Operator && (token->subtype == TokenSubType::Plus || token->subtype == TokenSubType::Minus)) {
        index++;
        std::string name;
        switch(token->subtype) {
            case TokenSubType::Plus:
                name = "plus"; break;
            case TokenSubType::Minus:
                name = "subtract"; break;
            default:
                return std::cout << "parse_low_binary_operation (/!\\ JSON corrupted) = nullptr\n", nullptr;
        }

        auto rhs = parse_high_binary_operation(index, tokens);
        std::cout << "parse_low_binary_operation ## index: " << index << '\n';

        if (!rhs || !lhs)
            return std::cout << "parse_low_binary_operation = nullptr\n", nullptr;

        std::cout << "parse_low_binary_operation = <" + name + ">\n";
        lhs = std::make_unique<BinaryOperator>(
            name,
            std::move(lhs),
            std::move(rhs)
        );

        if (index >= tokens.size())
            return std::move(lhs);
            
        token = &tokens.at(index);
        std::cout << "parse_low_binary_operation # index: " << index << '\n';
    }

    return std::move(lhs);
}

std::unique_ptr<AST> parse_high_binary_operation(std::size_t& index, std::vector<Token> const& tokens) {
    std::unique_ptr<AST> lhs = parse_term(index, tokens);

    if (index >= tokens.size())
        return std::move(lhs);

    auto* token = &tokens.at(index);
    std::cout << "parse_low_binary_operation # index: " << index << '\n';

    while(token->type == TokenType::Operator && (token->subtype == TokenSubType::Division || token->subtype == TokenSubType::Multiplication)) {
        index++;
        std::string name;
        switch(token->subtype) {
            case TokenSubType::Division:
                name = "division"; break;
            case TokenSubType::Multiplication:
                name = "multiplication"; break;
            default:
                return std::cout << "parse_high_binary_operation (/!\\ JSON corrupted) = nullptr\n", nullptr;
        }

        auto rhs = parse_term(index, tokens);
        std::cout << "parse_high_binary_operation ## index: " << index << '\n';

        if (!rhs || !lhs)
            return std::cout << "parse_high_binary_operation = nullptr\n", nullptr;

        std::cout << "parse_high_binary_operation = <" + name + ">\n";
        lhs = std::make_unique<BinaryOperator>(
            name,
            std::move(lhs),
            std::move(rhs)
        );

        if (index >= tokens.size())
            return std::move(lhs);
            
        token = &tokens.at(index);
        std::cout << "parse_low_binary_operation # index: " << index << '\n';
    }

    return std::move(lhs);
}

}}