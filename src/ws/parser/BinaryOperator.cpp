#include <ws/parser/BinaryOperator.hpp>

namespace ws::parser {

BinaryOperator::BinaryOperator(std::string const& name, std::unique_ptr<AST> lhs, std::unique_ptr<AST> rhs) : name(name), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

nlohmann::json BinaryOperator::compile() const {
    return {
        {"type", "operator." + name},
        {"lhs", lhs->compile() },
        {"rhs", rhs->compile() }
    };
}

std::ostream& BinaryOperator::dump(std::ostream& os) const {
    std::string symbol = '<' + name + '>';
    if (name == "plus") symbol = "+";
    else if (name == "subtract") symbol = "-";
    else if (name == "multiplication") symbol = "*";
    else if (name == "division") symbol = "/";

    return os << '(' << *lhs << ' ' << symbol << ' ' << *rhs << ')';
}

}