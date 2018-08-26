#include <ws/parser/Parser.hpp>

#include <ws/parser/AST.hpp>
#include <ws/parser/Number.hpp>
#include <ws/parser/BinaryOperator.hpp>

#include <iostream>

namespace ws { namespace parser {

std::unique_ptr<AST> parse_number(std::size_t& index, std::vector<Token> const& tokens);
std::unique_ptr<AST> parse_expression(std::size_t& index, std::vector<Token> const& tokens);
std::unique_ptr<AST> parse_binary_operation(std::size_t& index, std::vector<Token> const& tokens, std::unique_ptr<AST> lhs);

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

    if (token.type != TokenType::Litteral) 
        return std::cout << "parse_number = nullptr\n", nullptr;

    return std::cout << "parse_number = <number>\n", std::make_unique<Number>(
        token.content
    );
}

std::unique_ptr<AST> parse_expression(std::size_t& index, std::vector<Token> const& tokens) {
    auto number = parse_number(index, tokens);
    std::cout << "parse_expression # index: " << index << '\n';

    if (!number)
        return std::cout << "parse_expression = nullptr\n", nullptr;

    try {
        if (index >= tokens.size())
            return std::cout << "parse_expression = <number>\n", std::move(number);

        auto& token = tokens.at(index);
        std::cout << "parse_expression ## index: " << index << '\n';

        if (token.type != TokenType::Operator) 
            return std::cout << "parse_expression = nullptr\n", nullptr;

        return std::cout << "parse_expression = <parse_binary_operation...>\n", parse_binary_operation(index, tokens, std::move(number));
    } catch(std::out_of_range const&) {
        return std::cout << "parse_expression (/!\\ out_of_range) = nullptr\n", nullptr;
    }
}

std::unique_ptr<AST> parse_binary_operation(std::size_t& index, std::vector<Token> const& tokens, std::unique_ptr<AST> lhs) {
    auto& token = tokens.at(index++);
    std::cout << "parse_binary_operation # index: " << index << '\n';

    if (token.type != TokenType::Operator) 
        return std::cout << "parse_binary_operation = nullptr\n", nullptr;

    std::string name;
    switch(token.subtype) {
        case TokenSubType::Plus:
            name = "plus"; break;
        case TokenSubType::Minus:
            name = "minus"; break;
        case TokenSubType::Multiplication:
            name = "multiplication"; break;
        case TokenSubType::Division:
            name = "division"; break;
        default:
            return std::cout << "parse_binary_operation (/!\\ JSON corrupted) = nullptr\n", nullptr;
    }

    auto rhs = parse_expression(index, tokens);
    std::cout << "parse_binary_operation ## index: " << index << '\n';

    if (!rhs || !lhs)
        return std::cout << "parse_binary_operation = nullptr\n", nullptr; 

    return std::cout << "parse_binary_operation = <" + name + ">\n", std::make_unique<BinaryOperator>(
        name,
        std::move(lhs),
        std::move(rhs)
    );
}

}}