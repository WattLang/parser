#pragma once

#include <string>
#include <json.hpp>

namespace ws::parser {

class AST {
public:

    virtual nlohmann::json compile() const = 0;

    virtual std::ostream& dump(std::ostream& os) const = 0;

private:

};

inline std::ostream& operator<<(std::ostream& os, AST const& ast) {
    return ast.dump(os);
}

}