#pragma once

#include <ws/parser/AST.hpp>

namespace ws::parser {

class Number : public AST {
public:

    Number(std::string const& value);

    nlohmann::json compile() const override;

private:

    std::string value;

};

}