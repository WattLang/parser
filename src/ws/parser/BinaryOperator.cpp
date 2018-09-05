#include <ws/parser/BinaryOperator.hpp>

#include <json.hpp>

namespace ws { namespace parser {

BinaryOperator::BinaryOperator(std::string const& name, std::unique_ptr<AST> lhs, std::unique_ptr<AST> rhs) : name(name), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

std::string BinaryOperator::compile(unsigned indentation) const {
    nlohmann::json json = {
        {"type", "operator." + name},
        {"lhs", nlohmann::json::parse(lhs->compile(indentation)) },
        {"rhs", nlohmann::json::parse(rhs->compile(indentation)) }
    };
    return json.dump(indentation);
}

}}