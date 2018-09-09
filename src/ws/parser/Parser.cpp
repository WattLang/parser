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

AST_ptr term_to_AST(std::variant<std::tuple<Token, AST_ptr>, Token, /*std::tuple<Token, AST_ptr, Token>>*/ AST_ptr> expr) {
    switch(expr.index()) {
    case 0: // std::tuple<Token, AST_ptr>
        return std::make_unique<UnaryOperator>("negate", std::move(std::get<1>(std::get<0>(expr))));
    case 1: // Token
        return std::make_unique<Number>(std::get<1>(expr).content);
    case 2: // std::tuple<Token, AST_ptr, Token>
        return std::move(std::get<2>(expr));
    default:
        ws::module::errorln("WTF ? term_to_AST::expr should have an index of 0, 1 or 2... What is going on ?");
        return nullptr;
    }
}

AST_ptr factor_to_AST(AST_ptr lhs, std::vector<std::tuple<std::variant<Token, Token>, AST_ptr>> rhs) {
    for(auto& t : rhs) {
        auto name = std::visit([] (Token t) {
            if (t.subtype == TokenSubType::Division)
                return "division";
            return "multiplication";
        }, std::get<0>(t));
        lhs = std::make_unique<BinaryOperator>(name, std::move(lhs), std::move(std::get<1>(t)));
    }
    return std::move(lhs);
}

AST_ptr expr_to_AST(AST_ptr lhs, std::vector<std::tuple<std::variant<Token, Token>, AST_ptr>> rhs) {
    for(auto& t : rhs) {
        auto name = std::visit([] (Token t) {
            if (t.subtype == TokenSubType::Plus)
                return "plus";
            return "subtract";
        }, std::get<0>(t));
        lhs = std::make_unique<BinaryOperator>(name, std::move(lhs), std::move(std::get<1>(t)));
    }
    return std::move(lhs);
}

ParserResult parse(std::vector<Token> const& tokens) {

    /*
     * expr := factor  (('-' | '+') factor)*
     * factor := term (('*' | '/') term)*
     * term := '-' term | float | '(' expr ')'
     */

    auto float_eater     = log("float", eat(TokenType::Literal,     TokenSubType::Float));
    auto minus_eater     = log("'-'",   eat(TokenType::Operator,    TokenSubType::Minus));
    auto plus_eater      = log("'+'",   eat(TokenType::Operator,    TokenSubType::Plus));
    auto mult_eater      = log("'*'",   eat(TokenType::Operator,    TokenSubType::Multiplication));
    auto div_eater       = log("'/'",   eat(TokenType::Operator,    TokenSubType::Division));
    auto left_par_eater  = log("'('",   eat(TokenType::Parenthesis, TokenSubType::Left));
    auto right_par_eater = log("')'",   eat(TokenType::Parenthesis, TokenSubType::Right));

    Parser<AST_ptr> expr;
    Parser<AST_ptr> term;

    auto factor_operators = log(
        "'*' | '/'",
        mult_eater | div_eater);

    auto expr_operators = log(
        "'+' | '-'",
        plus_eater | minus_eater);

    auto term_negate = log(
        "'-' term", 
        minus_eater & ~term);

    auto term_parentherized_expr = log(
        "'(' expr ')'", 
        left_par_eater > ~expr < right_par_eater);

    term = log("term as AST", map(term_to_AST, log(
        "term := '-' term | float | '(' expr ')'", 
        term_negate | float_eater | term_parentherized_expr)));

    auto factor_rhs = log(
        "(('*' | '/') term)*",
        many(log(
            "('*' | '/') term", 
            factor_operators & term)));

    auto factor = log("factor as AST", mapI(factor_to_AST, log(
        "factor := term (('*' | '/') term)*",
        term & factor_rhs)));

    auto expr_rhs = log(
        "(('+' | '-') factor)*",
        many(log(
            "('+' | '-') factor", 
            expr_operators & factor)));

    expr = log("expr as AST", mapI(expr_to_AST, log(
        "expr := factor (('+' | '-') factor)*",
        factor & expr_rhs)));

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