#include <ws/parser/Number.hpp>

#include <json.hpp>

namespace ws { namespace parser {

Number::Number(std::string const& value) : value(value) {}

std::string Number::compile(unsigned indentation) const {
    nlohmann::json json = {
        {"type", "literal.float"},
        {"value", value}
    };
    return json.dump(indentation);
}

}}
