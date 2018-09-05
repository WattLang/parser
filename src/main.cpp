#include <iostream>
#include <cctype>
#include <optional>
#include <sstream>

#include <module/module.h>
#include <json.hpp>
#include <ws/parser/Parser.hpp>

std::vector<std::string> split_type(std::string const& s) {
    std::stringstream stream(s);
    std::vector<std::string> types;
    std::string type;
    while(std::getline(stream, type, '.'))
        types.emplace_back(type);
    return types;
}

std::optional<ws::parser::TokenType> parse_type(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s == "parenthesis")
        return ws::parser::TokenType::Parenthesis;
    if (s == "operator")
        return ws::parser::TokenType::Operator;
    if (s == "literal")
        return ws::parser::TokenType::Literal;

    return std::nullopt;
}

std::optional<ws::parser::TokenSubType> parse_subtype(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
    if (s == "left")
        return ws::parser::TokenSubType::Left;
    if (s == "right")
        return ws::parser::TokenSubType::Right;
    if (s == "plus")
        return ws::parser::TokenSubType::Plus;
    if (s == "minus")
        return ws::parser::TokenSubType::Minus;
    if (s == "multiplication")
        return ws::parser::TokenSubType::Multiplication;
    if (s == "division")
        return ws::parser::TokenSubType::Division;
    if (s == "float")
        return ws::parser::TokenSubType::Float;

    return std::nullopt;
}

std::vector<ws::parser::Token> parse_tokens(nlohmann::json const& json) {
    std::vector<ws::parser::Token> tokens;
    for(auto json_token : json) {
        auto raw_type = json_token["type"].get<std::string>();
        auto types = split_type(raw_type);
        if (types.size() != 2) {
            ws::println("Couldn't parse type ", raw_type);
            return {};
        }

        auto type = parse_type(types[0]);
        auto subtype = parse_subtype(types[1]);
        if (!type) {
            ws::println("Type ", types[0], " is not known");
            return {};
        }
        if (!subtype) {
            ws::println("Subtype ", types[1], " is not known");
            return {};
        }
        
        tokens.emplace_back(json_token["content"].get<std::string>(), *type, *subtype);
    }
    return tokens;
}

int main() {
    static constexpr std::uintmax_t buffer_size = 4;
    std::string raw_json = ws::receive_all(buffer_size);
    auto json = nlohmann::json::parse(raw_json);
    auto tokens = parse_tokens(json);
    auto result = ws::parser::parse(tokens);

    if (ws::parser::is_error(result)) {
        ws::warn(ws::parser::get_error(result)->what());
        return 1;
    }

    ws::pipeln(ws::parser::get_ast(result)->get()->compile(4));

    return 0;
}
