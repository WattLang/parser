#include <iostream>
#include <cctype>
#include <optional>
#include <sstream>

#include <module/module.h>
#include <json.hpp>
#include <ws/parser/Parser.hpp>
#include <ws/parser/token/TokenParser.hpp>

int main() {
    static constexpr std::uintmax_t buffer_size = 4;
    std::string raw_json = ws::module::receive_all(buffer_size);
    auto json = nlohmann::json::parse(raw_json);
    auto tokens_res = ws::parser::parse_tokens(json);

    if (auto err = std::get_if<std::unique_ptr<ws::parser::TokenParsingError>>(&tokens_res); err) {
        ws::module::errorln((*err)->what());
        return 1;
    }

    auto& tokens = std::get<std::vector<ws::parser::Token>>(tokens_res);

    for(auto const& token : tokens) {
        std::cout << token << '\n';
    }


    auto result = ws::parser::parse(tokens);


    if (ws::parser::is_error(result)) {
        ws::module::errorln(ws::parser::get_error(result)->what());
        return 1;
    }

    ws::module::pipeln(ws::parser::get_ast(result)->get()->compile().dump());

    return 0;
}
