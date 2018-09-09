#include <ws/parser/Parser.hpp>

#include <ws/parser/ast/AST.hpp>
#include <ws/parser/ast/Number.hpp>
#include <ws/parser/ast/BinaryOperator.hpp>
#include <ws/parser/ast/UnaryOperator.hpp>
#include <module/module.h>

#include <iostream>
#include <tuple>
#include <map>
#include <utility>
#include <type_traits>
#include <cstddef>

namespace ws::parser {

class TokenIterator {
public:
    using iterator = typename std::vector<Token>::const_iterator;

    TokenIterator(iterator begin, iterator end) : begin(begin), end(end) {}

    iterator get() const {
        return begin;
    }

    bool is_end_of_stream() const {
        return begin == end;
    }

    Token const& operator*() const {
        if (begin != end)
            return *begin;
        throw std::out_of_range("ouch");
    }

    Token const* operator->() const {
        return &**this;
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

template<typename T>
Parser<T> try_(Parser<T> const& p) {
    return [=] (TokenIterator& it) -> Result<T> {
        try {
            auto backup = it;
            auto res = p(backup);
            if (!has_failed(res)) {
                it = backup;
            }
            return std::move(res);
        } catch (std::out_of_range&) {
            return ParserError::error();
        }
    };
}

template<typename T>
struct is_tuple : std::bool_constant<false> {};

template<typename...Ts>
struct is_tuple<std::tuple<Ts...>> : std::bool_constant<true> {};

template<typename T>
constexpr auto is_tuple_v = is_tuple<T>::value;

template<typename A, typename B, bool = is_tuple_v<A>, bool = is_tuple_v<B>>
struct Combine;

template<typename A, typename B>
struct Combine<A, B, false, false> {
    using type = std::tuple<A, B>;
    static type combine(A a, B b) {
        return type { std::move(a), std::move(b) };
    }
};

template<typename...Args, typename...Brgs>
struct Combine<std::tuple<Args...>, std::tuple<Brgs...>, true, true> {
    using type = std::tuple<Args..., Brgs...>;
    static type combine(std::tuple<Args...> a, std::tuple<Brgs...> b) {
        return type { std::move(std::get<Args>(a))..., std::move(std::get<Brgs>(b))... };
    }
};

template<typename...Args, typename B>
struct Combine<std::tuple<Args...>, B, true, false> {
    using type = std::tuple<Args..., B>;
    static type combine(std::tuple<Args...> a, B b) {
        return type { std::move(std::get<Args>(a))..., std::move(b) };
    }
};

template<typename...Args, typename B>
struct Combine<B, std::tuple<Args...>, false, true> {
    using type = std::tuple<B, Args...>;
    static type combine(B b, std::tuple<Args...> a) {
        return type { std::move(b), std::move(std::get<Args>(a))... };
    }
};

template<typename A, typename B>
using Combine_t = typename Combine<A, B>::type;

template<typename A, typename B>
struct Either {
    using type = std::variant<A, B>;
    static type left(A a) {
        return type { std::in_place_index<0>, std::move(a) };
    }
    static type right(B b) {
        return type { std::in_place_index<1>, std::move(b) };
    }
};

template<typename A, typename B>
using Either_t = typename Either<A, B>::type;

template<typename A, typename B, template<typename, typename> typename Combinator = Combine>
Parser<typename Combinator<A, B>::type> operator & (Parser<A> pa, Parser<B> pb) {
    return [=] (TokenIterator& it) -> Result<typename Combinator<A, B>::type> { 
        auto a = pa(it);
        if (has_failed(a))
            return ParserError::error();

        auto b = pb(it);
        if (has_failed(b))
            return ParserError::error();

        return Combinator<A, B>::combine(std::move(std::get<A>(a)), std::move(std::get<B>(b)));
    };
}

template<typename A, typename B, template<typename, typename> typename Combinator = Either>
Parser<typename Combinator<A, B>::type> operator | (Parser<A> pa, Parser<B> pb) {
    return [=] (TokenIterator& it) -> Result<typename Combinator<A, B>::type> { 
        auto a = try_(pa)(it);

        if (!has_failed(a))
            return Combinator<A, B>::left(std::move(std::get<A>(a)));

        auto b = try_(pb)(it);

        if (!has_failed(b))
            return Combinator<A, B>::right(std::move(std::get<B>(b)));

        return ParserError::error();
    };
}

template<typename T, typename F>
Parser<std::invoke_result_t<F, T>> map(F && f, Parser<T> p) {
    return [f, p] (TokenIterator& it) -> Result<std::invoke_result_t<F, T>> {
        auto r = p(it);
        if (has_failed(r))
            return std::get<ParserError>(r);
        return f(std::move(std::get<T>(r)));
    };
}

template<typename F, typename T, size_t...Is >
decltype(auto) apply_tuple(F&& f, T t, std::index_sequence<Is...>) {
  return std::forward<F>(f)(std::move(std::get<Is>(t))...);
}

template<typename...Ts, typename F>
Parser<std::invoke_result_t<F, Ts...>> mapI(F && f, Parser<std::tuple<Ts...>> p) {
    return [f, p] (TokenIterator& it) -> Result<std::invoke_result_t<F, Ts...>> {
        auto r = p(it);
        if (has_failed(r)) 
            return std::get<ParserError>(r);
        return apply_tuple(f, std::move(std::get<1>(r)), std::make_index_sequence<sizeof...(Ts)>());
    };
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

template<typename T>
Parser<std::vector<T>> many(Parser<T> p) {
    return [=] (TokenIterator& it) {
        std::vector<T> res;
        while(true) {
            auto r = try_(p)(it);
            if (has_failed(r))
                return res;
            res.emplace_back(std::move(std::get<T>(r)));
        }
    };
}

template<typename T>
Parser<std::tuple<T, std::vector<T>>> some(Parser<T> p) {
    return p & many(p);
}
/*
template<typename T>
Parser<std::vector<T>> some_vec(Parser<T> p) {
    return map([] (std::pair<T, std::vector<T>> p) {
        std::vector<T> vec { p.first };
        vec.reserve(p.second.size() + 1);
        vec.insert(vec.end(), p.second.begin(), p.second.end());
        return vec;
    }, some(p));
}
*/
template<typename T, typename...Ts>
Parser<T> join (Parser<std::variant<T, Ts...>> parser) {
    return map([] (auto r) -> T { return std::visit([] (auto a) { return std::move(a); }, std::move(r)); }, parser);
}

template<typename...Ts, std::size_t...Is>
std::ostream& print_tuple(std::ostream& os, std::tuple<Ts...> const& tuple, std::index_sequence<Is...>) {
    (std::initializer_list<int>){((os << (Is == 0 ? "" : ", ") << std::get<Is>(tuple)), 0)...};
    return os;
}

template<typename...Ts>
std::ostream& operator<<(std::ostream& os, std::tuple<Ts...> const& tuple) {
    os << "<";
    return print_tuple(os, tuple, std::make_index_sequence<sizeof...(Ts)>()) << ">";
}

template<typename T>
std::ostream& operator<<(std::ostream& os, std::vector<T> const& vector) {
    bool first = true;
    os << "[";
    for(auto const& t : vector) {
        if (!first)
            os << ", ";
        os << t;
        first = false;
    }
    return os << "]";
}

template<typename T>
std::ostream& operator<<(std::ostream& os, std::unique_ptr<T> const& ptr) {
    return os << "*" << *ptr;
}


template<typename T>
std::ostream& operator<<(std::ostream& os, std::optional<T> const& opt) {
    if (opt)
        return os << "$" << *opt;
    return os << "$null";
}

template<typename...Ts>
std::ostream& operator<<(std::ostream& os, std::variant<Ts...> const& variant) {
    os << "|" << variant.index() << ": ";
    return std::visit([&os] (auto const& v) -> std::ostream& { return os << v; }, variant) << "|";
}

template<typename T>
Parser<T> log(std::string const& name, Parser<T> const& parser) {
    return [=] (TokenIterator& it) {
        ws::module::noticeln("• Begin <", name, ">");
        auto res = parser(it);
        if (has_failed(res)) {
            ws::module::warnln("• <", name, "> failed: ", std::get<ParserError>(res).what());
        } else {
            ws::module::println(
                ws::module::colour::fg::green, ws::module::style::bold, "[O] ", ws::module::style::reset, 
                "• <", name, "> succeed: ", ws::module::colour::fg::cyan, std::get<T>(res));
        }
        return std::move(res);
    };
}

template<typename T>
Parser<std::optional<T>> optional(Parser<T> const& p) {
    return [=] (TokenIterator& it) -> Result<std::optional<T>> {
        auto res = try_(p)(it);
        if (has_failed(res))
            return std::nullopt;
        return std::optional<T>(std::move(std::get<T>(res)));
    };
}

template<typename T>
Parser<T> ref(Parser<T>& p) {
    return [&] (TokenIterator& it) {
        return std::move(p(it));
    };
}

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
                log("", ref(low_binary_operation)) & 
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
        TokenIterator it(tokens.begin(), tokens.end());
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