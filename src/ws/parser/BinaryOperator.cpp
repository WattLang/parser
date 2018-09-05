#include <ws/parser/BinaryOperator.hpp>

namespace ws { namespace parser {

BinaryOperator::BinaryOperator(std::string const& name, std::unique_ptr<AST> lhs, std::unique_ptr<AST> rhs) : name(name), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

nlohmann::json BinaryOperator::compile() const {
    return {
        {"type", "operator." + name},
        {"lhs", lhs->compile() },
        {"rhs", rhs->compile() }
    };
}

}}