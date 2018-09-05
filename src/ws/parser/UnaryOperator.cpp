#include <ws/parser/UnaryOperator.hpp>

#include <json.hpp>

namespace ws { namespace parser {

UnaryOperator::UnaryOperator(std::string const& name, std::unique_ptr<AST> operand) : name(name), operand(std::move(operand)) {}

std::string UnaryOperator::compile(unsigned indentation) const {
    nlohmann::json json = {
        {"type", "operator." + name},
        {"operand", nlohmann::json::parse(operand->compile(indentation)) }
    };
    return json.dump(indentation);
}

}}