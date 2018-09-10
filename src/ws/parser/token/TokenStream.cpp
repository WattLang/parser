#include <ws/parser/token/TokenStream.hpp>

namespace ws::parser {

TokenStream::TokenStream(TokenStream::iterator begin, TokenStream::iterator end) : begin(begin), end(end) {}

bool TokenStream::is_end_of_stream() const {
    return begin == end;
}

Token const& TokenStream::operator*() const {
    if (begin != end)
        return *begin;
    throw std::out_of_range("ouch");
}

Token const* TokenStream::operator->() const {
    return &**this;
}

TokenStream& TokenStream::operator++() {
    begin++;
    return *this;
}

TokenStream& TokenStream::operator--() {
    begin--;
    return *this;
}

TokenStream TokenStream::operator++(int) {
    TokenStream tmp(*this);
    ++(*this);
    return tmp;
}

TokenStream TokenStream::operator--(int) {
    TokenStream tmp(*this);
    --(*this);
    return tmp;
}


}