#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "hyh_grammar_parser.hpp"
#include "raw/raw_syntax.hpp"

namespace hyh {

    class Lexer {
    public:
        explicit Lexer(std::string_view input);

        GrammarParser::symbol_type next();

    private:
        [[nodiscard]] bool atEnd() const;
        [[nodiscard]] char peek() const;
        char advance();
        [[nodiscard]] Lexeme makeLexeme(Lexeme::Kind kind, std::size_t begin, std::size_t end, std::size_t line,
                                        std::size_t column) const;
        [[nodiscard]] Lexeme makeNumber(std::size_t begin, std::size_t end, std::size_t line, std::size_t column) const;
        [[nodiscard]] Lexeme makeString(std::size_t begin, std::size_t end, std::size_t line, std::size_t column) const;
        void skipHorizontalWhitespace();

        std::string input_;
        const char* cursor_ = nullptr;
        const char* marker_ = nullptr;
        const char* limit_ = nullptr;
        std::size_t offset_ = 0;
        std::size_t line_ = 1;
        std::size_t column_ = 1;
    };

} // namespace hyh
