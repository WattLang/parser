#pragma once

#include <vector>

#include <ws/parser/token/Token.hpp>

namespace ws::parser {

class TokenStream {
public:
    using iterator = typename std::vector<Token>::const_iterator;

    TokenStream(iterator begin, iterator end);

    bool is_end_of_stream() const;
    Token const& operator*() const;
    Token const* operator->() const;
    TokenStream& operator++();
    TokenStream& operator--();
    TokenStream operator++(int);
    TokenStream operator--(int);

private:

    iterator begin, end;

};


}