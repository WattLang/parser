#include <ws/parser/Parser.hpp>

#include <ws/parser/AST.hpp>
#include <ws/parser/Number.hpp>
#include <ws/parser/BinaryOperator.hpp>
#include <ws/parser/UnaryOperator.hpp>

#include <iostream>
#include <map>

namespace ws { namespace parser {

ParserResult parse_number(std::size_t& index, std::vector<Token> const& tokens);
ParserResult parse_term(std::size_t& index, std::vector<Token> const& tokens);
ParserResult parse_expression(std::size_t& index, std::vector<Token> const& tokens);
ParserResult parse_low_binary_operation(std::size_t& index, std::vector<Token> const& tokens);
ParserResult parse_high_binary_operation(std::size_t& index, std::vector<Token> const& tokens);
ParserResult parse_binary_operation(std::size_t& index, std::vector<Token> const& tokens, ParserResult(*next)(std::size_t&, std::vector<Token> const&), std::map<std::pair<TokenType, TokenSubType>, std::string> const& names);

ParserResult parse(std::vector<Token> const& tokens) {
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

ParserResult parse_number(std::size_t& index, std::vector<Token> const& tokens) {
    auto& token = tokens.at(index++);

    if (token.type != TokenType::Litteral || token.subtype != TokenSubType::Float) 
        return ParserError::expected({"number litteral"});

    return std::make_unique<Number>(
        token.content
    );
}

ParserResult parse_term(std::size_t& index, std::vector<Token> const& tokens) {
    auto& token = tokens.at(index);

    switch(token.type) {
        case TokenType::Litteral: 
            if (token.subtype == TokenSubType::Float)
                return parse_number(index, tokens);
            else 
                return ParserError::expected({"number litteral", "minus"});

        case TokenType::Operator: {
            if (token.subtype != TokenSubType::Minus) 
                return ParserError::expected({"number litteral", "minus"});

            index++;
            std::string name = tokens.at(index-1).content;
            auto operand = parse_term(index, tokens);
            if (is_error(operand))
                return operand;
            return std::make_unique<UnaryOperator>(name, std::move(*get_ast(operand)));
        }

        default:
            return ParserError::expected({"number litteral", "minus"});
    }
}

ParserResult parse_expression(std::size_t& index, std::vector<Token> const& tokens) {
    return parse_low_binary_operation(index, tokens);
}

ParserResult parse_low_binary_operation(std::size_t& index, std::vector<Token> const& tokens) {
    return parse_binary_operation(index, tokens, parse_high_binary_operation, {
        {{TokenType::Operator, TokenSubType::Plus}, "plus"},
        {{TokenType::Operator, TokenSubType::Minus}, "subtract"}
    });
}

ParserResult parse_high_binary_operation(std::size_t& index, std::vector<Token> const& tokens) {
    return parse_binary_operation(index, tokens, parse_term, {
        {{TokenType::Operator, TokenSubType::Multiplication}, "multiplication"},
        {{TokenType::Operator, TokenSubType::Division}, "division"}
    });
}

ParserResult parse_binary_operation(
    std::size_t& index, 
    std::vector<Token> const& tokens, 
    ParserResult(*next)(std::size_t&, std::vector<Token> const&), 
    std::map<std::pair<TokenType, TokenSubType>, std::string> const& names
) {
    auto lhs = next(index, tokens);

    if (is_error(lhs))
        return lhs;

    if (index >= tokens.size())
        return lhs;

    std::optional<std::string> name;
    auto get_name = [&names](TokenType type, TokenSubType subtype) -> std::optional<std::string> {
        auto it = names.find({type, subtype});
        if (it == names.end())
            return std::nullopt;
        return it->second;
    };

    {
        auto* token = &tokens.at(index);
        name = get_name(token->type, token->subtype);
    }

    while(name) {
        index++;
        auto rhs = next(index, tokens);

        if (is_error(rhs))
            return rhs;

        lhs = std::make_unique<BinaryOperator>(
            *name,
            std::move(*get_ast(lhs)),
            std::move(*get_ast(rhs))
        );

        if (index >= tokens.size())
            return lhs;
            
        auto& token = tokens.at(index);
        name = get_name(token.type, token.subtype);
    }

    return lhs;
}

}}