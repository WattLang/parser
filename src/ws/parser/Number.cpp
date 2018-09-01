#include <ws/parser/Number.hpp>

namespace ws { namespace parser {

Number::Number(std::string const& value) : value(value) {}

std::string Number::compile(unsigned indentation) const {
    return 
                                        "{\n" +
        std::string(indentation, ' ') + "  type:\"literal.float\",\n" +
        std::string(indentation, ' ') + "  value:\"" + value + "\"\n" +
        std::string(indentation, ' ') + "}";
}

}}