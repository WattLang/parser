#include <ws/parser/Parser.hpp>
#include <ws/parser/ParserInternal.hpp>
#include <ws/parser/token/TokenStream.hpp>

#include <ws/parser/ast/AST.hpp>
#include <ws/parser/ast/Number.hpp>
#include <ws/parser/ast/BinaryOperator.hpp>
#include <ws/parser/ast/UnaryOperator.hpp>
#include <module/module.h>

namespace ws::parser {

template<typename T>
ParserResult to_result(Result<T> r) {
    if (has_failed(r))
        return std::get<ParserError>(r);
    return std::move(std::get<T>(r));
}

ParserResult parse(std::vector<Token> const& tokens) {
    auto float_eater = log("float", eat(TokenType::Literal, TokenSubType::Float));
    auto minus_eater = log("'-'", eat(TokenType::Operator, TokenSubType::Minus));
    auto plus_eater = log("'+'", eat(TokenType::Operator, TokenSubType::Plus));
    auto mult_eater = log("'*'", eat(TokenType::Operator, TokenSubType::Multiplication));
    auto div_eater = log("'/'", eat(TokenType::Operator, TokenSubType::Division));
    auto left_par_eater = log("'('", eat(TokenType::Parenthesis, TokenSubType::Left));
    auto right_par_eater = log("')'", eat(TokenType::Parenthesis, TokenSubType::Right));

    Parser<AST_ptr> low_binary_operation;

    auto term_parser = map([] (std::variant<std::tuple<std::vector<Token>, Token>, std::tuple<Token, AST_ptr, Token>> expr) -> AST_ptr {
        if (expr.index() == 0) {
            auto& tokens = std::get<0>(expr);
            AST_ptr term = std::make_unique<Number>(std::get<1>(tokens).content);
            for(auto t : std::get<0>(tokens))
                term = std::make_unique<UnaryOperator>("negate", std::move(term));
            return std::move(term);
        }
        return std::move(std::get<1>(std::move(std::get<1>(expr))));
    }, 
        log("term := '-'* float | '(' expr ')'",
            log("'-'* float", 
                log("'-'*", many(minus_eater)) & 
                float_eater) | 
            log("'(' expr ')'", 
                left_par_eater & 
                log("expr", ws::parser::ref(low_binary_operation)) & 
                right_par_eater
            )
        )
    );

    auto high_binary_operation = mapI([] (AST_ptr lhs, std::vector<std::tuple<std::variant<Token, Token>, AST_ptr>> rhs) -> AST_ptr {
        for(auto& t : rhs) {
            auto name = std::visit([] (Token t) {
                if (t.subtype == TokenSubType::Division)
                    return "division";
                return "multiplication";
            }, std::get<0>(t));
            lhs = std::make_unique<BinaryOperator>(name, std::move(lhs), std::move(std::get<1>(t)));
        }
        return std::move(lhs);
    }, 
        log("high_binary := term (('*' | '/') term)*", 
            term_parser & 
            log("(('*' | '/') term)*", 
                many(
                    log("('*' | '/') term",
                        log("'*' | '/'", mult_eater | div_eater) & 
                        term_parser
                    )
                )
            )
        )
    );

    low_binary_operation = mapI([] (AST_ptr lhs, std::vector<std::tuple<std::variant<Token, Token>, AST_ptr>> rhs) -> AST_ptr {
        for(auto& t : rhs) {
            auto name = std::visit([] (Token t) {
                if (t.subtype == TokenSubType::Plus)
                    return "plus";
                return "subtract";
            }, std::get<0>(t));
            lhs = std::make_unique<BinaryOperator>(name, std::move(lhs), std::move(std::get<1>(t)));
        }
        return std::move(lhs);
    }, 
        log("low_binary := high_binary (('-' | '+') high_binary)*", 
            high_binary_operation & 
            log("(('-' | '+') high_binary)*", 
                many(
                    log("('-' | '+') high_binary",
                        log("'-' | '+'", minus_eater | plus_eater) & 
                        high_binary_operation
                    )
                )
            )
        )
    );

    auto expr = log("expr := low_binary", low_binary_operation);

    try {
        TokenStream it(tokens.begin(), tokens.end());
        auto res = expr(it);
        if (has_failed(res))
            return std::get<ParserError>(res);
        if (!it.is_end_of_stream())
            return ParserError::expected({"end of stream"});
            
        return std::move(std::get<AST_ptr>(res));

    } catch(std::out_of_range const&) {
        return ParserError::error();
    }
}

}