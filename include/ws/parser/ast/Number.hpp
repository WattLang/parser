#pragma once

#include <ws/parser/ast/AST.hpp>

namespace ws::parser {

class Number : public AST {
public:

    Number(std::string const& value);

    nlohmann::json compile() const override;

    std::ostream& dump(std::ostream& os) const override;

private:

    std::string value;

};

}