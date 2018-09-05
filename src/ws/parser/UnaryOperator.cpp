#include <ws/parser/UnaryOperator.hpp>

namespace ws { namespace parser {

UnaryOperator::UnaryOperator(std::string const& name, std::unique_ptr<AST> operand) : name(name), operand(std::move(operand)) {}

nlohmann::json UnaryOperator::compile() const {
    return {
        {"type", "operator." + name},
        {"operand", operand->compile() }
    };
}

}}