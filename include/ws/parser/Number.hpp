#pragma once

#include <ws/parser/AST.hpp>

namespace ws { namespace parser {

class Number : public AST {
public:

    Number(std::string const& value);

    std::string compile(unsigned indentation) const override;

private:

    std::string value;

};

}}