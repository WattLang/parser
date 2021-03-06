#pragma once

#include <memory>

#include <ws/parser/ast/AST.hpp>

namespace ws::parser {

class BinaryOperator : public AST {
public:

    BinaryOperator(std::string const& name, std::unique_ptr<AST> lhs, std::unique_ptr<AST> rhs);

    nlohmann::json compile() const override;

    std::ostream& dump(std::ostream& os) const override;

private:

    std::string name;
    std::unique_ptr<AST> lhs;
    std::unique_ptr<AST> rhs;

};

}