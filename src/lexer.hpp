#pragma once

#include <string>

#include "parser.hpp"

namespace hyh {

class Lexer {
public:
    explicit Lexer(std::string input);

    Parser::symbol_type next();

private:
    std::string input_;
    const char* cursor_ = nullptr;
    const char* marker_ = nullptr;

    Parser::location_type location_;

    Parser::symbol_type classifyHebrewWord(const char* begin, const char* end);
};

} // namespace hyh
