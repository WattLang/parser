#pragma once

#include <memory>

#include <ws/parser/AST.hpp>

namespace ws { namespace parser {

class BinaryOperator : public AST {
public:

    BinaryOperator(std::string const& name, std::unique_ptr<AST> lhs, std::unique_ptr<AST> rhs);

    nlohmann::json compile() const override;

private:

    std::string name;
    std::unique_ptr<AST> lhs;
    std::unique_ptr<AST> rhs;

};

}}