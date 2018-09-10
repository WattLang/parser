#include <ws/parser/ast/Number.hpp>

namespace ws::parser {

Number::Number(std::string const& value) : value(value) {}

nlohmann::json Number::compile() const {
    return {
        {"type", "literal.float"},
        {"value", value}
    };
}

std::ostream& Number::dump(std::ostream& os) const {
    return os << value;
}

}