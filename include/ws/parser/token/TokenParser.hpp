#pragma once

#include <ws/parser/token/Token.hpp>

#include <json.hpp>

#include <memory>
#include <string>
#include <initializer_list>
#include <variant>

namespace ws::parser {

using json_t = nlohmann::json;

class TokenParsingError {
public:
    virtual std::string what() const = 0;
};

class RootNotArray : public TokenParsingError {
public:
    virtual std::string what() const override;

};

class MissingKey : public TokenParsingError {
public:
    MissingKey(std::string const& key);

    virtual std::string what() const override;

private:
    std::string key;

};

class TypeMismatch : public TokenParsingError {
public:
    TypeMismatch(std::string const& key, std::string const& expecting_type, std::string const& got_type);

    virtual std::string what() const override;

private:
    std::string key, expecting_type, got_type;

};

class TokenTypeUnknown : public TokenParsingError {
public:
    TokenTypeUnknown(std::string const& group_type, std::vector<std::string> const& expecting_types, std::string const& got_type);

    virtual std::string what() const override;

private:
    std::string group_type, got_type;
    std::vector<std::string> expecting_types;

};

class EmptyTokenType : public TokenParsingError {
public:

    virtual std::string what() const override;

};

class UnreachablePosition : public TokenParsingError {
public:
    UnreachablePosition(std::string const& key, json_t::number_integer_t position);

    virtual std::string what() const override;

private:
    std::string key;
    json_t::number_integer_t position;

};

std::variant<Token, std::unique_ptr<TokenParsingError>> parse_token(json_t const& json);
std::variant<std::vector<ws::parser::Token>, std::unique_ptr<TokenParsingError>> parse_tokens(json_t const& json);

}