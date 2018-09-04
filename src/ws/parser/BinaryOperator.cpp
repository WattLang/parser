#include <ws/parser/BinaryOperator.hpp>

namespace ws::parser {

BinaryOperator::BinaryOperator(std::string const& name, std::unique_ptr<AST> lhs, std::unique_ptr<AST> rhs) : name(name), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

std::string BinaryOperator::compile(unsigned indentation) const {
    return
                                        "{\n" +
        std::string(indentation, ' ') + "  \"type\":\"operator." + name + "\",\n" +
        std::string(indentation, ' ') + "  \"lhs\":" + lhs->compile(indentation + 2) + ",\n" +
        std::string(indentation, ' ') + "  \"rhs\":" + rhs->compile(indentation + 2) + "\n" +
        std::string(indentation, ' ') + "}";
}

}