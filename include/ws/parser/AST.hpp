#pragma once

#include <string>
#include <json.hpp>

namespace ws::parser {

class AST {
public:

    virtual nlohmann::json compile() const = 0;

private:

};

}