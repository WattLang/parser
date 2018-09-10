#include <ws/parser/ast/UnaryOperator.hpp>

namespace ws::parser {

UnaryOperator::UnaryOperator(std::string const& name, std::unique_ptr<AST> operand) : name(name), operand(std::move(operand)) {}

nlohmann::json UnaryOperator::compile() const {
    return {
        {"type", "operator." + name},
        {"operand", operand->compile() }
    };
}

std::ostream& UnaryOperator::dump(std::ostream& os) const {
    std::string symbol = '<' + name + '>';
    if (name == "negate") symbol = "-";

    return os << symbol << *operand;
}

}