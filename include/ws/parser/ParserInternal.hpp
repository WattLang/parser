#pragma once

#include <tuple>
#include <vector>
#include <optional>
#include <variant>

#include <ws/parser/ParserResult.hpp>
#include <ws/parser/token/TokenStream.hpp>
#include <ws/parser/token/Token.hpp>

#include <module/module.h>

namespace ws::parser {

template<typename T>
using Result = std::variant<ParserError, T>;

template<typename T>
using Parser = std::function<Result<T>(TokenStream&)>; 

template<typename T>
bool has_failed(Result<T> const& r) {
    return std::holds_alternative<ParserError>(r);
}

template<typename T>
Parser<T> try_(Parser<T> const& p) {
    return [=] (TokenStream& it) -> Result<T> {
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
    static type combine(A&& a, B&& b) {
        return type { std::move(a), std::move(b) };
    }
};

template<typename...Args, typename...Brgs>
struct Combine<std::tuple<Args...>, std::tuple<Brgs...>, true, true> {
    using type = std::tuple<Args..., Brgs...>;
    static type combine(std::tuple<Args...>&& a, std::tuple<Brgs...>&& b) {
        return type { std::move(std::get<Args>(a))..., std::move(std::get<Brgs>(b))... };
    }
};

template<typename...Args, typename B>
struct Combine<std::tuple<Args...>, B, true, false> {
    using type = std::tuple<Args..., B>;
    static type combine(std::tuple<Args...>&& a, B&& b) {
        return type { std::move(std::get<Args>(a))..., std::move(b) };
    }
};

template<typename...Args, typename B>
struct Combine<B, std::tuple<Args...>, false, true> {
    using type = std::tuple<B, Args...>;
    static type combine(B&& b, std::tuple<Args...>&& a) {
        return type { std::move(b), std::move(std::get<Args>(a))... };
    }
};

template<typename A, typename B>
using Combine_t = typename Combine<A, B>::type;

template<typename A, typename B>
struct Either {
    using type = std::variant<A, B>;
    static type left(A&& a) {
        return type { std::in_place_index<0>, std::move(a) };
    }
    static type right(B&& b) {
        return type { std::in_place_index<1>, std::move(b) };
    }
};

template<typename A, typename B>
using Either_t = typename Either<A, B>::type;

template<typename A, typename B>
Parser<typename Combine<A, B>::type> operator & (Parser<A> const& pa, Parser<B> const& pb) {
    return [=] (TokenStream& it) -> Result<typename Combine<A, B>::type> { 
        auto a = pa(it);
        if (has_failed(a))
            return ParserError::error();

        auto b = pb(it);
        if (has_failed(b))
            return ParserError::error();

        return Combine<A, B>::combine(std::move(std::get<A>(a)), std::move(std::get<B>(b)));
    };
}

template<typename A, typename B>
Parser<typename Either<A, B>::type> operator | (Parser<A> const& pa, Parser<B> const& pb) {
    return [=] (TokenStream& it) -> Result<typename Either<A, B>::type> { 
        auto a = try_(pa)(it);

        if (!has_failed(a))
            return Either<A, B>::left(std::move(std::get<A>(a)));

        auto b = try_(pb)(it);

        if (!has_failed(b))
            return Either<A, B>::right(std::move(std::get<B>(b)));

        return ParserError::error();
    };
}

template<typename T, typename F>
Parser<std::invoke_result_t<F, T>> map(F && f, Parser<T> const& p) {
    return [f, p] (TokenStream& it) -> Result<std::invoke_result_t<F, T>> {
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
Parser<std::invoke_result_t<F, Ts...>> mapI(F && f, Parser<std::tuple<Ts...>> const& p) {
    return [f, p] (TokenStream& it) -> Result<std::invoke_result_t<F, Ts...>> {
        auto r = p(it);
        if (has_failed(r)) 
            return std::get<ParserError>(r);
        return apply_tuple(f, std::move(std::get<1>(r)), std::make_index_sequence<sizeof...(Ts)>());
    };
}

Parser<Token> eat(TokenType type, TokenSubType subtype) {
    return [=] (TokenStream& it) -> Result<Token> {
        auto t = *it;
        if (t.type == type && t.subtype == subtype) {
            ++it;
            return t;
        }
        return ParserError::error();
    };
}

template<typename T>
Parser<std::vector<T>> many(Parser<T> const& p) {
    return [=] (TokenStream& it) {
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
Parser<std::tuple<T, std::vector<T>>> some(Parser<T> const& p) {
    return p & many(p);
}

template<typename T, typename...Ts>
Parser<T> join (Parser<std::variant<T, Ts...>> const& parser) {
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
    return [=] (TokenStream& it) {
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
    return [=] (TokenStream& it) -> Result<std::optional<T>> {
        auto res = try_(p)(it);
        if (has_failed(res))
            return std::nullopt;
        return std::optional<T>(std::move(std::get<T>(res)));
    };
}

template<typename T>
Parser<T> ref(Parser<T> const& p) {
    return [&p] (TokenStream& it) {
        return std::move(p(it));
    };
}

}