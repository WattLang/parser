#include <ws/parser/Parser.hpp>

#include <ws/parser/AST.hpp>
#include <ws/parser/Number.hpp>
#include <ws/parser/BinaryOperator.hpp>
#include <ws/parser/UnaryOperator.hpp>

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
    using type = std::tuple<A, B>;
    static type combine(A a, B b) {
        return type { std::move(a), std::move(b) };
    }
};

template<typename...Args, typename B>
struct Combine<std::tuple<Args...>, B> {
    using type = std::tuple<Args..., B>;
    static type combine(std::tuple<Args...> a, B b) {
        return type { std::move(std::get<Args>(a))..., std::move(b) };
    }
};

template<typename...Args, typename B>
struct Combine<B, std::tuple<Args...>> {
    using type = std::tuple<B, Args...>;
    static type combine(B b, std::tuple<Args...> a) {
        return type { std::move(b), std::move(std::get<Args>(a))... };
    }
};

template<typename...Args, typename...Brgs>
struct Combine<std::tuple<Args...>, std::tuple<Brgs...>> {
    using type = std::tuple<Args..., Brgs...>;
    static type combine(std::tuple<Args...> a, std::tuple<Brgs...> b) {
        return type { std::move(std::get<Args>(a))..., std::move(std::get<Brgs>(b))... };
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
    static std::enable_if_t<std::is_same_v<A, B>, A> join(type t) {
        return std::visit([] (auto a) {
            return std::move(a);
        }, std::move(t));
    }
};

template<typename A, typename B>
using Either_t = typename Either<A, B>::type;

template<typename A, typename B, template<typename, typename> typename Combinator = Combine>
Parser<typename Combinator<A, B>::type> operator & (Parser<A> pa, Parser<B> pb) {
    return [=] (TokenIterator& it) -> Result<typename Combinator<A, B>::type> { 
        std::cerr << "operator &: pa(it)...\n";
        auto a = pa(it);
        std::cerr << "operator &: pb(it)...\n";
        auto b = pb(it);

        std::cerr << "operator &: checking\n";
        if (has_failed(a) || has_failed(b)) {
            std::cerr << "operator &: failed\n";
            return ParserError::error();
        }
        std::cerr << "operator &: didn't failed\n";
        return Combinator<A, B>::combine(std::move(std::get<A>(a)), std::move(std::get<B>(b)));
    };
}

template<typename A, typename B, template<typename, typename> typename Combinator = Either>
Parser<typename Combinator<A, B>::type> operator | (Parser<A> pa, Parser<B> pb) {
    return [=] (TokenIterator& it) -> Result<typename Combinator<A, B>::type> { 
        auto it_a = it;
        std::cerr << "operator |: pa(it_a)\n";
        try {
            auto a = pa(it_a);
            std::cerr << "operator |: checking a\n";
            if (!has_failed(a)) {
                std::cerr << "operator |: pa didn't failed\n";
                it = it_a;
                return Combinator<A, B>::left(std::move(std::get<A>(a)));
            }
        } catch(std::out_of_range&) {}
        auto it_b = it;
        std::cerr << "operator |: pb(it_b)\n";
        try {
            auto b = pb(it_b);
            std::cerr << "operator |: checking b\n";
            if (!has_failed(b)) {
                std::cerr << "operator |: pb didn't failed\n";
                it = it_b;
                return Combinator<A, B>::right(std::move(std::get<B>(b)));
            }
        } catch(std::out_of_range&) {}
        std::cerr << "operator |: failed\n";
        return ParserError::error();
    };
}

template<typename T, typename F>
Parser<std::invoke_result_t<F, T>> map(F && f, Parser<T> p) {
    return [f, p] (TokenIterator& it) -> Result<std::invoke_result_t<F, T>> {
        std::cerr << "map: pa(it)...\n";
        auto r = p(it);
        std::cerr << "map: checking, index is " << r.index() << "\n";
        if (has_failed(r)) {
            std::cerr << "map: failed\n";
            return std::get<ParserError>(r);
        }
        std::cerr << "map: applying f on index " << r.index() << "\n";
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
        std::cerr << "eat: get token...\n";
        auto t = *it;
        std::cerr << "eat: checking\n";
        std::cerr << "Wants [" << (int)type << ", " << (int)subtype << "] have [" << (int)t.type << ", " << (int)t.subtype << "]\n";
        if (t.type == type && t.subtype == subtype) {
            ++it;
            std::cerr << "eat: advancing and returning [" << (int)type << ", " << (int)subtype << "]\n";
            Result<Token> r = t;
            std::cerr << "eat: index is " << r.index() << "\n";
            return r;
        }
        std::cerr << "eat: failed [" << (int)type << ", " << (int)subtype << "]\n";
        Result<Token> r = ParserError::error();
        std::cerr << "eat: index is " << r.index() << "\n";
        return r;
    };
}

template<typename T>
Parser<std::vector<T>> many(Parser<T> p) {
    return [=] (TokenIterator& it) {
        std::vector<T> res;
        std::cerr << "many: starting...\n";
        while(true) {
            auto r = p(it);
            if (has_failed(r)) {
                std::cerr << "many: p failed, returns " << res.size() << "\n";
                return res;
            }
            std::cerr << "many: +1\n";
            res.emplace_back(std::move(std::get<T>(r)));
        }
    };
}

template<typename T>
Parser<std::pair<T, std::vector<T>>> some(Parser<T> p) {
    return p & many(p);
}

template<typename T>
Parser<std::vector<T>> some_vec(Parser<T> p) {
    return map([] (std::pair<T, std::vector<T>> p) {
        std::vector<T> vec { p.first };
        vec.reserve(p.second.size() + 1);
        vec.insert(vec.end(), p.second.begin(), p.second.end());
        return vec;
    }, some(p));
}

template<typename T, typename...Ts>
Parser<T> join (Parser<std::variant<T, Ts...>> parser) {
    return map([] (auto r) -> T { return std::visit([] (auto a) { return std::move(a); }, std::move(r)); }, parser);
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
    auto float_eater = eat(TokenType::Literal, TokenSubType::Float);
    auto minus_eater = eat(TokenType::Operator, TokenSubType::Minus);
    auto plus_eater = eat(TokenType::Operator, TokenSubType::Plus);
    auto mult_eater = eat(TokenType::Operator, TokenSubType::Multiplication);
    auto div_eater = eat(TokenType::Operator, TokenSubType::Division);
 
    auto term_parser = mapI([] (std::vector<Token> tokens, Token number) -> AST_ptr {
        AST_ptr term = std::make_unique<Number>(number.content);
        for(auto t : tokens)
            term = std::make_unique<UnaryOperator>(t.content, std::move(term));
        return std::move(term);
    }, many(minus_eater) & float_eater);

    auto high_binary_operation = mapI([] (AST_ptr lhs, std::variant<Token, Token> op, AST_ptr rhs) -> AST_ptr {
        auto name = std::visit([] (Token t) {
            if (t.subtype == TokenSubType::Division)
                return "division";
            return "multiplication";
        }, op);
        return std::make_unique<BinaryOperator>(name, std::move(lhs), std::move(rhs));
    }, term_parser & ((mult_eater | div_eater) & term_parser));

    auto pass_high_binary_operation = join(high_binary_operation | term_parser);

    auto low_binary_operation = mapI([] (AST_ptr lhs, std::variant<Token, Token> op, AST_ptr rhs) -> AST_ptr {
        auto name = std::visit([] (Token t) {
            if (t.subtype == TokenSubType::Plus)
                return "plus";
            return "subtract";
        }, op);
        return std::make_unique<BinaryOperator>(name, std::move(lhs), std::move(rhs));
    }, pass_high_binary_operation & ((minus_eater | plus_eater) & pass_high_binary_operation));

    auto pass_low_binary_operation = join(low_binary_operation | high_binary_operation);

    try {
        TokenIterator it(tokens.begin(), tokens.end());
        std::cerr << "Parsing...\n";
        auto res = pass_low_binary_operation(it);

        std::cerr << "Checking...\n";
        if (has_failed(res))
            return std::get<ParserError>(res);
        std::cerr << "OK...\n";
            
        std::cerr << "Returning...\n";
        return std::move(std::get<AST_ptr>(res));

    } catch(std::out_of_range const&) {
        return ParserError::error();
    }
}

}