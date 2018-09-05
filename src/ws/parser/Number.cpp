#include <ws/parser/Number.hpp>

namespace ws::parser {

Number::Number(std::string const& value) : value(value) {}

nlohmann::json Number::compile() const {
    return {
        {"type", "literal.float"},
        {"value", value}
    };
}

}