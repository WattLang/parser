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

/* Parser Combinator:
 * -> Build bigger parser from smaller one

 *    Parser<T> = Result<T> (TokenStream&)
 *    Result<T> = ParserError or T

 * Parser:

 * eat (TokenType, TokenSubType) -> Parser<Token>
 *    Comsume the next token of the stream if the type/subtype match, or returns an error, the stream is always comsumed

 * try_ (Parser<A>) -> Parser<A>
 *    Try to run the parser, if it fails rollback the stream where it was, still returns what the parser returned

 * operator & (Parser<A>, Parser<B>) -> Parser<std::tuple<A, B>>
 *    Run sequencially both parser, returns their result in a tuple, or an error
 *    Note:
 *        If A or B is a tuple, it will concatenate the tuple
 *        So operator & (Parser<std::tuple<A, B>>, Parser<std::tuple<C, A>>) -> Parser<std::tuple<A, B, C, A>>

 * operator | (Parser<A>, Parser<B>) -> Parser<std::variant<A, B>>
 *    Run the first parser, if it fails run the second parser, if it fails returns an error
 *    Note:
 *        If A or B is a variant, it will concatenate the variant
 *        So operator | (Parser<std::variant<A, B>>, Parser<std::variant<C, A>>) -> Parser<std::variant<A, B, C, A>>

 * many (Parser<A>) -> Parser<std::vector<A>>
 *    Accumulate the result of the parser until it fails, this parser never fails, since it can returns an empty vector

 * some (Parser<A>) -> Parser<std::tuple<A, std::vector<A>>>
 *    Combination of itself and many, will parse A at least once, if the first iteration fails, an error is resturned

 * operator ~ (Parser<A>) -> Parser<A>
 *    Equivalent of 'ref'

 * operator < (Parser<A>, Parser<B>) -> Parser<A>
 *    Equivalent of 'a & b' but discard the second result

 * operator > (Parser<A>, Parser<B>) -> Parser<B>
 *    Equivalent of 'a & b' but discard the first result

 * optional (Parser<A>) -> Parser<std::optional<A>>
 *    Run the parser, if it fails return an empty optional, can't fail

 * ref (Parser<A>) -> Parser<A>
 *    By default parser are copied, but when you need recursion, you have to declare the parser first then referenced it with ref(parser)
 *    Example:
 *        Parser<Thing> parenthesis_expr;
 *        parenthesis_expr = left_par & ref(parenthesis_expr) & right_par | expr;

 * map (B(A), Parser<A>) -> Parser<B>
 *    Run the callback on the result of the parser if it has succeeded

 * mapI (B(A...), Parser<std::tuple<A...>>) -> Parser<B>
 *    Equivalent of 'map' but unpack each element of a tuple

 * log (std::string, Parser<A>) -> Parser<A>
 *    Print a message when the parser is run, and when it succed/fail, returns the result of that parser, the type A need to implement operato<<(std::ostream&)

 * log (std::size_t&, std::string, Parser<A>) -> Parser<A>
 *    Equivalent of log but will use spaces to indent the code, it also increament it for the future logs

 * join (Parser<std::variant<A...>>) -> Parser<B>
 *    Run the parser and convert the result into B, All types in the variant need to be convertible to B

 */





/*
 * Internal Helpers
 */

namespace internal {





template<typename T>
struct is_tuple : std::bool_constant<false> {};

template<typename...Ts>
struct is_tuple<std::tuple<Ts...>> : std::bool_constant<true> {};

template<typename T>
constexpr auto is_tuple_v = is_tuple<T>::value;





// Combine::combine will put both type in a tuple, nested tuples will be flatten

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





template<typename T>
struct is_variant : std::bool_constant<false> {};

template<typename...Ts>
struct is_variant<std::variant<Ts...>> : std::bool_constant<true> {};

template<typename T>
constexpr auto is_variant_v = is_variant<T>::value;





// Create a bigger variant from another
// 'V' is the target variant
// 'I' is the starting index, should be 0 at the start
// 'Offset' between the target variant and the current variant, use in 'std::in_place_index<I + Offset>'
// 'Ts...' are the types in the source variant

template<typename V, std::size_t I, size_t Offset, typename...Ts>
V create_variant(std::variant<Ts...>&& v) {
    if constexpr (I + 1 < sizeof...(Ts))
        if (v.index() != I)
            return create_variant<V, I + 1, Offset, Ts...>(std::move(v));
    return V { std::in_place_index<I + Offset>, std::move(std::get<I>(v)) };
}





// Either::left or Either::right will construct the variant<A, B>, any nested variants will be flatten

template<typename A, typename B, bool = is_variant_v<A>, bool = is_variant_v<B>>
struct Either;

template<typename A, typename B>
struct Either<A, B, false, false> {
    using type = std::variant<A, B>;
    static type left(A&& a) {
        return type { std::in_place_index<0>, std::move(a) };
    }
    static type right(B&& b) {
        return type { std::in_place_index<1>, std::move(b) };
    }
};

template<typename...Args, typename B>
struct Either<std::variant<Args...>, B, true, false> {
    using type = std::variant<Args..., B>;

    static type left(std::variant<Args...>&& a) {
        return create_variant<type, 0, 0, Args...>(std::move(a));
    }
    static type right(B&& b) {
        return type { std::in_place_index<sizeof...(Args)>, std::move(b) };
    }
};

template<typename A, typename...Brgs>
struct Either<A, std::variant<Brgs...>, false, true> {
    using type = std::variant<A, Brgs...>;

    static type left(A&& a) {
        return type { std::in_place_index<0>, std::move(a) };
    }
    static type right(std::variant<Brgs...>&& b) {
        return create_variant<type, 0, 1, Brgs...>(std::move(b));
    }
};

template<typename...Args, typename...Brgs>
struct Either<std::variant<Args...>, std::variant<Brgs...>, true, true> {
    using type = std::variant<Args..., Brgs...>;

    static type left(std::variant<Args...>&& a) {
        return create_variant<type, 0, 0, Args...>(std::move(a));
    }
    static type right(std::variant<Brgs...>&& b) {
        return create_variant<type, 0, sizeof...(Args), Brgs...>(std::move(b));
    }
};

template<typename A, typename B>
using Either_t = typename Either<A, B>::type;





// Pass all elements of the tuple to the function

template<typename F, typename T, size_t...Is >
decltype(auto) apply_tuple(F&& f, T t, std::index_sequence<Is...>) {
  return std::forward<F>(f)(std::move(std::get<Is>(t))...);
}





template<typename...Ts, std::size_t...Is>
std::ostream& print_tuple(std::ostream& os, std::tuple<Ts...> const& tuple, std::index_sequence<Is...>) {
    (std::initializer_list<int>){((os << (Is == 0 ? "" : ", ") << std::get<Is>(tuple)), 0)...};
    return os;
}





};





/*
 * General definitions
 */

template<typename T>
using Result = std::variant<ParserError, T>;

template<typename T>
using Parser = std::function<Result<T>(TokenStream&)>; 





template<typename T>
bool has_failed(Result<T> const& r) {
    return std::holds_alternative<ParserError>(r);
}





/*
 * try_ (Parser<A>) -> Parser<A>
 *    Try to run the parser, if it fails rollback the stream where it was, still returns what the parser returned
 */
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





/*
 * operator & (Parser<A>, Parser<B>) -> Parser<std::tuple<A, B>>
 *    Run sequencially both parser, returns their result in a tuple, or an error
 *    Note:
 *        If A or B is a tuple, it will concatenate the tuple
 *        So operator & (Parser<std::tuple<A, B>>, Parser<std::tuple<C, A>>) -> Parser<std::tuple<A, B, C, A>>
 */
template<typename A, typename B>
Parser<typename internal::Combine<A, B>::type> operator & (Parser<A> const& pa, Parser<B> const& pb) {
    return [=] (TokenStream& it) -> Result<typename internal::Combine<A, B>::type> { 
        auto a = pa(it);
        if (has_failed(a))
            return ParserError::error();

        auto b = pb(it);
        if (has_failed(b))
            return ParserError::error();

        return internal::Combine<A, B>::combine(std::move(std::get<A>(a)), std::move(std::get<B>(b)));
    };
}





/*
 * operator | (Parser<A>, Parser<B>) -> Parser<std::variant<A, B>>
 *    Run the first parser, if it fails run the second parser, if it fails returns an error
 *    Note:
 *        If A or B is a variant, it will concatenate the variant
 *        So operator | (Parser<std::variant<A, B>>, Parser<std::variant<C, A>>) -> Parser<std::variant<A, B, C, A>>
 */
template<typename A, typename B>
Parser<typename internal::Either<A, B>::type> operator | (Parser<A> const& pa, Parser<B> const& pb) {
    return [=] (TokenStream& it) -> Result<typename internal::Either<A, B>::type> { 
        auto a = try_(pa)(it);

        if (!has_failed(a))
            return internal::Either<A, B>::left(std::move(std::get<A>(a)));

        auto b = try_(pb)(it);

        if (!has_failed(b))
            return internal::Either<A, B>::right(std::move(std::get<B>(b)));

        return ParserError::error();
    };
}





/*
 * map (B(A), Parser<A>) -> Parser<B>
 *    Run the callback on the result of the parser if it has succeeded
 */
template<typename T, typename F>
Parser<std::invoke_result_t<F, T>> map(F && f, Parser<T> const& p) {
    return [f, p] (TokenStream& it) -> Result<std::invoke_result_t<F, T>> {
        auto r = p(it);
        if (has_failed(r))
            return std::get<ParserError>(r);
        return f(std::move(std::get<T>(r)));
    };
}





/*
 * mapI (B(A...), Parser<std::tuple<A...>>) -> Parser<B>
 *    Equivalent of 'map' but unpack each element of a tuple
 */
template<typename...Ts, typename F>
Parser<std::invoke_result_t<F, Ts...>> mapI(F && f, Parser<std::tuple<Ts...>> const& p) {
    return [f, p] (TokenStream& it) -> Result<std::invoke_result_t<F, Ts...>> {
        auto r = p(it);
        if (has_failed(r)) 
            return std::get<ParserError>(r);
        return internal::apply_tuple(f, std::move(std::get<1>(r)), std::make_index_sequence<sizeof...(Ts)>());
    };
}





/*
 * eat (TokenType, TokenSubType) -> Parser<Token>
 *    Comsume the next token of the stream if the type/subtype match, or returns an error, the stream is always comsumed
 */
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





/*
 * many (Parser<A>) -> Parser<std::vector<A>>
 *    Accumulate the result of the parser until it fails, this parser never fails, since it can returns an empty vector
 */
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





/*
 * some (Parser<A>) -> Parser<std::tuple<A, std::vector<A>>>
 *    Combination of itself and many, will parse A at least once, if the first iteration fails, an error is resturned
 */
template<typename T>
Parser<std::tuple<T, std::vector<T>>> some(Parser<T> const& p) {
    return p & many(p);
}





/*
 * join (Parser<std::variant<A...>>) -> Parser<B>
 *    Run the parser and convert the result into B, All types in the variant need to be convertible to B
 */
template<typename T, typename...Ts>
Parser<T> join (Parser<std::variant<T, Ts...>> const& parser) {
    return map([] (auto r) -> T { return std::visit([] (auto a) { return std::move(a); }, std::move(r)); }, parser);
}





/*
 * log (std::string, Parser<A>) -> Parser<A>
 *    Print a message when the parser is run, and when it succed/fail, returns the result of that parser, the type A need to implement operato<<(std::ostream&)
 */
template<typename T>
Parser<T> log(std::string const& name, Parser<T> const& parser) {
    return [=] (TokenStream& it) {
        ws::module::noticeln("• Begin <", ws::module::style::bold, name, ws::module::style::reset, ">");
        try {
            auto res = parser(it);

            if (has_failed(res)) {
                ws::module::warnln("• <", ws::module::style::bold, name, ws::module::style::reset, "> failed: ", std::get<ParserError>(res).what());
            } else {
                ws::module::println(
                    ws::module::colour::fg::green, ws::module::style::bold, "[_] ", ws::module::style::reset, 
                    "• <", ws::module::style::bold, name, ws::module::style::reset, "> succeed: ", ws::module::colour::fg::cyan, std::get<T>(res));
            }
            return std::move(res);
        } catch(...) {
            ws::module::warnln("• <", ws::module::style::bold, name, ws::module::style::reset, "> threw an exception");
            throw;
        }
    };
}





/*
 * log (std::size_t&, std::string, Parser<A>) -> Parser<A>
 *    Equivalent of log but will use spaces to indent the code, it also increament it for the future logs
 */
template<typename T>
Parser<T> log(std::size_t& spaces, std::string const& name, Parser<T> const& parser) {
    return [&spaces, name, parser] (TokenStream& it) {
        ws::module::noticeln(std::string(spaces, ' '), "• Begin <", ws::module::style::bold, name, ws::module::style::reset, ">");

        spaces += 2;
        try {
            auto res = parser(it);
            spaces -= 2;

            if (has_failed(res)) {
                ws::module::warnln(std::string(spaces, ' '), "• <", ws::module::style::bold, name, ws::module::style::reset, "> failed: ", std::get<ParserError>(res).what());
            } else {
                ws::module::println(
                    ws::module::colour::fg::green, ws::module::style::bold, "[_] ", std::string(spaces, ' '), ws::module::style::reset, 
                    "• <", ws::module::style::bold, name, ws::module::style::reset, "> succeed: ", ws::module::colour::fg::cyan, std::get<T>(res));
            }
            return std::move(res);
        } catch(...) {
            spaces -= 2;
            ws::module::warnln(std::string(spaces, ' '), "• <", ws::module::style::bold, name, ws::module::style::reset, "> threw an exception");
            throw;
        }
    };
}





/*
 * optional (Parser<A>) -> Parser<std::optional<A>>
 *    Run the parser, if it fails return an empty optional, can't fail
 */
template<typename T>
Parser<std::optional<T>> optional(Parser<T> const& p) {
    return [=] (TokenStream& it) -> Result<std::optional<T>> {
        auto res = try_(p)(it);
        if (has_failed(res))
            return std::nullopt;
        return std::optional<T>(std::move(std::get<T>(res)));
    };
}





/*
 * ref (Parser<A>) -> Parser<A>
 *    By default parser are copied, but when you need recursion, you have to declare the parser first then referenced it with ref(parser)
 *    Example:
 *        Parser<Thing> parenthesis_expr;
 *        parenthesis_expr = left_par & ref(parenthesis_expr) & right_par | expr;
 */
template<typename T>
Parser<T> ref(Parser<T> const& p) {
    return [&p] (TokenStream& it) {
        return std::move(p(it));
    };
}





/*
 * operator ~ (Parser<A>) -> Parser<A>
 *    Equivalent of 'ref'
 */
template<typename T>
Parser<T> operator~(Parser<T> const& p) {
    return ref(p);
}





/*
 * operator > (Parser<A>, Parser<B>) -> Parser<B>
 *    Equivalent of 'a & b' but discard the first result
 */
template<typename A, typename B>
Parser<B> operator > (Parser<A> const& pa, Parser<B> const& pb) {
    return mapI([] (auto, auto res) { return res; }, pa & pb);
}






/*
 * operator < (Parser<A>, Parser<B>) -> Parser<A>
 *    Equivalent of 'a & b' but discard the second result
 */
template<typename A, typename B>
Parser<A> operator < (Parser<A> const& pa, Parser<B> const& pb) {
    return mapI([] (auto res, auto) { return res; }, pa & pb);
}





/*
 * operator<< for
 *    std::tuple
 *    std::optional
 *    std::unique_ptr
 *    std::variant
 *    std::vector
 */





template<typename...Ts>
std::ostream& operator<<(std::ostream& os, std::tuple<Ts...> const& tuple) {
    os << "<";
    return internal::print_tuple(os, tuple, std::make_index_sequence<sizeof...(Ts)>()) << ">";
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





}