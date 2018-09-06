#include <ws/parser/Parser.hpp>

#include <ws/parser/AST.hpp>
#include <ws/parser/Number.hpp>
#include <ws/parser/BinaryOperator.hpp>
#include <ws/parser/UnaryOperator.hpp>

#include <iostream>
#include <map>

namespace ws::parser {

class TokenIterator {
public:
    using iterator = typename std::vector<Token>::const_iterator;

    TokenIterator(iterator begin, iterator end) : begin(begin), end(end) {}

    iterator get() const {
        return begin;
    }

    Token const& operator*() const {
        if (begin != end)
            return *begin;
        throw std::out_of_range("ouch");
    }

    Token const* operator->() const {
        if (begin != end)
            return &*begin;
        throw std::out_of_range("ouch");
    }

    TokenIterator& operator++() {
        begin++;
        return *this;
    }

    TokenIterator& operator--() {
        begin--;
        return *this;
    }

    TokenIterator operator++(int) {
        TokenIterator tmp(*this);
        ++(*this);
        return tmp;
    }

    TokenIterator operator--(int) {
        TokenIterator tmp(*this);
        --(*this);
        return tmp;
    }

private:

    iterator begin, end;

};
template<typename T>
using Result = std::variant<ParserError, T>;

template<typename T>
using Parser = std::function<Result<T>(TokenIterator&)>; 

template<typename T>
bool has_failed(Result<T> const& r) {
    return std::holds_alternative<ParserError>(r);
}

template<typename A, typename B>
struct Combine {
    using type = std::pair<A, B>;
    static type combine(A a, B b) {
        return { std::move(a), std::move(b) };
    }
};

template<typename A, typename B>
using Combine_t = typename Combine<A, B>::type;

template<typename A, typename B>
struct Either {
    using type = std::variant<A, B>;
    static type left(A a) {
        return { std::move(a) };
    }
    static type right(B b) {
        return { std::move(b) };
    }
};

template<typename A, typename B>
using Either_t = typename Either<A, B>::type;

template<typename A, typename B, template<typename, typename> typename Combinator = Combine>
Parser<typename Combinator<A, B>::type> operator & (Parser<A> const& pa, Parser<B> const& pb) {
    return [&] (TokenIterator& it) -> Result<typename Combinator<A, B>::type> { 
        auto a = pa(it);
        auto b = pb(it);
        if (has_failed(a) || has_failed(b))
            return ParserError::error();
        return Combinator<A, B>::combine(std::move(std::get<A>(a)), std::move(std::get<B>(b)));
    };
}

template<typename A, typename B, template<typename, typename> typename Combinator = Either>
Parser<typename Combinator<A, B>::type> operator | (Parser<A> const& pa, Parser<B> const& pb) {
    return [&] (TokenIterator& it) { 
        auto it_a = it;
        auto a = pa(it_a);
        if (!has_failed(a)) {
            it = it_a;
            return Combinator<A, B>::left(std::move(a));
        }
        auto it_b = it;
        auto b = pb(it_b);
        if (!has_failed(b)) {
            it = it_b;
            return Combinator<A, B>::right(std::move(b));
        }
        return ParserError::error();
    };
}

template<typename T, typename F>
Parser<std::invoke_result_t<F, T>> map(F && f, Parser<T> p) {
    return [&] (TokenIterator& it) -> Result<std::invoke_result_t<F, T>> {
        auto r = p(it);
        if (has_failed(r)) 
            return std::get<ParserError>(r);
        return std::forward<F&&>(f)(std::move(std::get<T>(r)));
    };
}

template<std::size_t... Is, typename T, typename F>
Parser<std::invoke_result_t<F, T>> mapI_impl(std::index_sequence<Is...>, F && f, Parser<T> p) {
    return [&] (TokenIterator& it) -> Result<std::invoke_result_t<F, T>> {
        auto r = p(it);
        if (has_failed(r)) 
            return std::get<ParserError>(r);
        return std::forward<F&&>(f)(std::move(std::get<Is>(r))...);
    };
}

template<std::size_t I, typename T, typename F>
Parser<std::invoke_result_t<F, T>> mapI(F && f, Parser<T> p) {
    return mapI_impl(std::make_index_sequence<I>{}, std::forward<F&&>(f), p);
}

Parser<Token> eat(TokenType type, TokenSubType subtype) {
    return [=] (TokenIterator& it) -> Result<Token> {
        auto t = *it;
        if (t.type == type && t.subtype == subtype) {
            ++it;
            return t;
        }
        return ParserError::error();
    };
}

/*
template<typename T>
Parser<std::vector<T>> many(Parser<T> p) {
    std::vector<T> res;
    auto f = [p, &res] (Token const& t) {
        auto r = p(t);
        if (has_failed(r))
            return res;
        res.emplace_back(std::move(std::get<T>(r)));
        return f;
    };
}*/

template<typename T>
ParserResult to_result(Result<T> r) {
    if (has_failed(r))
        return std::get<ParserError>(r);
    return std::move(std::get<T>(r));
}

class ParserSource {
public:

    ParserSource(std::vector<Token> const& tokens) : tokens(tokens), index(0) {}
    ParserSource(std::vector<Token> const& tokens, std::size_t i) : tokens(tokens), index(i) {}

    std::optional<Token> current() const {
        if (index >= tokens.size())
            return {};
        return tokens[index];
    }

    std::optional<Token> at(int offset) const {
        int i = static_cast<int>(index) + offset;
        if (i >= static_cast<int>(tokens.size()) || i < 0)
            return {};
        return tokens[i];
    }

    std::vector<Token> tokens;
    std::size_t index;
};

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
    //ParserSource parser(tokens, index++);
    auto& token = tokens.at(index++);


    //auto token = parser.eat(TokenType::Literal, TokenSubType::Float);


    if (token.type != TokenType::Literal || token.subtype != TokenSubType::Float)
        return ParserError::expected({"number litteral"});

    return std::make_unique<Number>(
        token.content
    );

/*
    if (std::holds_alternative<ParserError>(token))
        return std::get<ParserError>(token);
    return std::make_unique<Number>(
        std::get<std::vector<Token>>(token)[0].content
    );*/
/*
    auto number = map([] (Token const& t) {
        return std::make_unique<Number>(
            t.content
        );
    }, token);
    return to_result(std::move(number));  
    */

    //return to_result(std::move(num_parser(tokens[index++])));
  
}

    template<typename...Args>
    class A;

ParserResult parse_term(std::size_t& index, std::vector<Token> const& tokens) {

    auto token_num = eat(TokenType::Literal, TokenSubType::Float);
    auto num_parser = map([] (Token const& t) {
        return std::make_unique<Number>(
            t.content
        );
    }, token_num);

    auto token_sub = eat(TokenType::Operator, TokenSubType::Minus);
    auto sub_parser = map([] (std::pair<Token, Token> t) {
        return std::make_unique<UnaryOperator>(
            t.first.content,
            std::make_unique<Number>(t.second.content)
        );
    }, token_sub & token_num);

    auto term_parser = sub_parser | num_parser;

    TokenIterator it(tokens.begin() + index, tokens.end());
    auto res = term_parser(it);
    index = std::distance(tokens.begin(), it.get());

    if (has_failed(res))
        return std::get<ParserError>(res);

    auto var = std::get<std::variant<std::unique_ptr<UnaryOperator>, std::unique_ptr<Number>>>(res);
    
    //A<decltype(res)> _;
    
    return std::visit(
        [] (auto a) -> std::unique_ptr<AST> { return std::move(a); },
        std::move(var)
    );
    /*
    
    auto& token = tokens.at(index);

    switch(token.type) {
        case TokenType::Literal:
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
    }*/
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

}