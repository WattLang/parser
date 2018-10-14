#include <ws/parser/token/TokenParser.hpp>

#include <unordered_map>
#include <sstream>

namespace ws::parser {

std::string RootNotArray::what() const {
    return "Root should be an array of tokens";
}



MissingKey::MissingKey(std::string const& key) : key(key) {}

std::string MissingKey::what() const {
    return "Missing key '" + key  + "'";
}



TypeMismatch::TypeMismatch(std::string const& key, std::string const& expecting_type, std::string const& got_type)
    : key(key), expecting_type(expecting_type), got_type(got_type) {}

std::string TypeMismatch::what() const {
    return "Mismatch type on key '" + key + "', got '" + got_type + "' but '" + expecting_type + "' was expected";
}



TokenTypeUnknown::TokenTypeUnknown(std::string const& group_type, std::vector<std::string> const& expecting_types, std::string const& got_type)
    : group_type(group_type), got_type(got_type), expecting_types(expecting_types) {}

std::string TokenTypeUnknown::what() const {
    std::string res = "Token's ";
    if (group_type.empty())
        res += "type '" + got_type + "'";
    else
        res += "subtype '" + got_type + "' of '" + group_type + "'";

    res += " is unknown. One of the following was expected:";
    for(auto s : expecting_types)
        res += "\n\t- '" + s + "'";
    return res;
}



std::string EmptyTokenType::what() const {
    return "Token's type cannot be empty";
}



UnreachablePosition::UnreachablePosition(std::string const& key, json_t::number_integer_t position)
    : key(key), position(position)
{
    this->key[0] = std::toupper(this->key[0]);
}

std::string UnreachablePosition::what() const {
    return key + " is unreachable (" + std::to_string(position) + ")";
}



std::string type_as_string(json_t::value_t type) {
    switch (type) {
        case json_t::value_t::null:            return "null";
        case json_t::value_t::object:          return "object";
        case json_t::value_t::array:           return "array";
        case json_t::value_t::string:          return "string";
        case json_t::value_t::boolean:         return "boolean";
        case json_t::value_t::number_integer:  return "number integer";
        case json_t::value_t::number_unsigned: return "number unsigned";
        case json_t::value_t::number_float:    return "number float";
        default:                               return "unknown";
    }
}



std::variant<std::string, std::unique_ptr<TokenParsingError>> parse_content(json_t const& json) {
    static constexpr auto key = "content";
    if (json.count(key) <= 0)
        return std::make_unique<MissingKey>(key);

    auto content_json = json[key];
    if (!content_json.is_string())
        return std::make_unique<TypeMismatch>(key, type_as_string(json_t::value_t::string), type_as_string(content_json.type()));

    return content_json.get<std::string>();
}



std::variant<std::size_t, std::unique_ptr<TokenParsingError>> parse_position(json_t const& json, std::string const& key) {
    if (json.count(key) <= 0)
        return std::make_unique<MissingKey>(key);

    auto content_json = json[key];
    if (!content_json.is_number_integer())
        return std::make_unique<TypeMismatch>(key, type_as_string(json_t::value_t::number_integer), type_as_string(content_json.type()));

    if (!content_json.is_number_unsigned())
        return std::make_unique<UnreachablePosition>(key, content_json.get<json_t::number_integer_t>());
        
    return content_json.get<std::size_t>();
}



std::vector<std::string> split_type(std::string const& s) {
    std::stringstream stream(s);
    std::vector<std::string> types;
    std::string type;
    while(std::getline(stream, type, '.'))
        types.emplace_back(type);
    return types;
}



template<typename T>
std::vector<std::string> keys(std::unordered_map<std::string, T> const& map) {
    std::vector<std::string> strings;
    strings.reserve(map.size());
    for(auto&&[key, _] : map)
        strings.emplace_back(key);
    return strings;
}



std::variant<std::pair<TokenType, TokenSubType>, std::unique_ptr<TokenParsingError>> parse_type(json_t const& json) {
    static constexpr auto key = "type";

    if (json.count(key) <= 0)
        return std::make_unique<MissingKey>(key);

    auto content_json = json[key];
    if (!content_json.is_string())
        return std::make_unique<TypeMismatch>(key, type_as_string(json_t::value_t::string), type_as_string(content_json.type()));

    auto types = split_type(content_json.get<std::string>());
    if (types.size() <= 0)
        return std::make_unique<EmptyTokenType>();

    static std::unordered_map<std::string, std::unordered_map<std::string, std::pair<TokenType, TokenSubType>>> map_type {
        {"literal", {
            {"float", 
                {TokenType::Literal, TokenSubType::Float}
            }
        }},
        {"parenthesis", {
            {"left", 
                {TokenType::Parenthesis, TokenSubType::Left}
            },
            {"right", 
                {TokenType::Parenthesis, TokenSubType::Right}
            }
        }},
        {"operator", {
            {"plus", 
                {TokenType::Operator, TokenSubType::Plus}
            },
            {"minus", 
                {TokenType::Operator, TokenSubType::Minus}
            },
            {"multiplication", 
                {TokenType::Operator, TokenSubType::Multiplication}
            },
            {"division", 
                {TokenType::Operator, TokenSubType::Division}
            },
        }},
    };

    if (map_type.count(types[0]) <= 0)
        return std::make_unique<TokenTypeUnknown>("", keys(map_type), types[0]);
    
    auto const& map_subtype = map_type.find(types[0])->second;

    if (types.size() <= 1)
        return std::make_unique<TokenTypeUnknown>(types[0], keys(map_subtype), "");

    if (map_subtype.count(types[1]) <= 0)
        return std::make_unique<TokenTypeUnknown>(types[0], keys(map_subtype), types[1]);

    return map_subtype.find(types[1])->second;
}



SingleTokenParserResult parse_token(json_t const& json) {
    auto content = parse_content(json);
    auto type = parse_type(json);
    auto line = parse_position(json, "line");
    auto column = parse_position(json, "column");

    if (auto* error = std::get_if<std::unique_ptr<TokenParsingError>>(&content); error)
        return std::move(*error);
    if (auto* error = std::get_if<std::unique_ptr<TokenParsingError>>(&type); error)
        return std::move(*error);
    if (auto* error = std::get_if<std::unique_ptr<TokenParsingError>>(&line); error)
        return std::move(*error);
    if (auto* error = std::get_if<std::unique_ptr<TokenParsingError>>(&column); error)
        return std::move(*error);
    
    return Token {
        std::get<std::string>(content),
        std::get<std::pair<TokenType, TokenSubType>>(type).first,
        std::get<std::pair<TokenType, TokenSubType>>(type).second,
        std::get<std::size_t>(line),
        std::get<std::size_t>(column)
    };
}



TokenParserResult parse_tokens(json_t const& json) {
    if (!json.is_array())
        return std::make_unique<RootNotArray>();

    std::vector<ws::parser::Token> tokens;

    for(auto json_token : json) {
        auto res = ws::parser::parse_token(json_token);

        if (auto err = get_error(res); err) {
            std::cout << (*err)->what() << '\n';
            return std::move(*err);
        }

        tokens.emplace_back(*get_token(res));
    }
    return tokens;
}



bool is_error(SingleTokenParserResult const& res) {
    return get_error(res) != nullptr;
}

bool is_error(TokenParserResult const& res) {
    return get_error(res) != nullptr;
}



std::unique_ptr<TokenParsingError> const* get_error(SingleTokenParserResult const& error) {
    return std::get_if<std::unique_ptr<TokenParsingError>>(&error);
}

Token const* get_token(SingleTokenParserResult const& error) {
    return std::get_if<Token>(&error);
}

std::unique_ptr<TokenParsingError>* get_error(SingleTokenParserResult& error) {
    return std::get_if<std::unique_ptr<TokenParsingError>>(&error);
}

Token* get_token(SingleTokenParserResult& error) {
    return std::get_if<Token>(&error);
}



std::unique_ptr<TokenParsingError> const* get_error(TokenParserResult const& error) {
    return std::get_if<std::unique_ptr<TokenParsingError>>(&error);
}

std::vector<Token> const* get_tokens(TokenParserResult const& error) {
    return std::get_if<std::vector<Token>>(&error);
}

std::unique_ptr<TokenParsingError>* get_error(TokenParserResult& error) {
    return std::get_if<std::unique_ptr<TokenParsingError>>(&error);
}

std::vector<Token>* get_tokens(TokenParserResult& error) {
    return std::get_if<std::vector<Token>>(&error);
}


}