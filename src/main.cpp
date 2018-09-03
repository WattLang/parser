#include <iostream>
#include <cctype>
#include <optional>

#include <ws/Module.hpp>
#include <json.hpp>
#include <ws/parser/Parser.hpp>

std::optional<ws::parser::TokenType> parse_type(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
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
        auto type = parse_type(json_token["type"].get<std::string>());
        auto subtype = parse_subtype(json_token["subtype"].get<std::string>());
        if (!type) {
            ws::println("Couldn't parse `",json_token["type"].get<std::string>(), "`");
            return {};
        }
        if (!subtype) {
            ws::println("Couldn't parse `",json_token["subtype"].get<std::string>(), "`");
            return {};
        }
        tokens.emplace_back(json_token["content"].get<std::string>(), *type, *subtype);
    }
    return tokens;
}

int main() {
    ws::Receiver rec(/* buffer size */ 4);

    std::string raw_json;

    int error_code = 0;

    auto cb = [&raw_json, &error_code] (std::string const& buffer, int, bool end) {
        raw_json += buffer;

        if (end) {
            auto json = nlohmann::json::parse(raw_json);
            auto tokens = parse_tokens(json);
            auto result = ws::parser::parse(tokens);

            if (ws::parser::is_error(result)) {
                ws::Logger().warn(ws::parser::get_error(result)->what());
                error_code = 1;
                return;
            }

            ws::pipeln(ws::parser::get_ast(result)->get()->compile(0));
            return;
        }
    };

    rec.read(cb);
    return error_code;
}
