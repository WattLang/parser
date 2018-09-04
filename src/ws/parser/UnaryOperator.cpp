#include <ws/parser/UnaryOperator.hpp>

namespace ws { namespace parser {

UnaryOperator::UnaryOperator(std::string const& name, std::unique_ptr<AST> operand) : name(name), operand(std::move(operand)) {}

std::string UnaryOperator::compile(unsigned indentation) const {
    return 
                                        "{\n" +
        std::string(indentation, ' ') + "  \"type\":\"operator." + name + "\",\n" +
        std::string(indentation, ' ') + "  \"operand\":" + operand->compile(indentation + 2) + ",\n" +
        std::string(indentation, ' ') + "}";
}

}}