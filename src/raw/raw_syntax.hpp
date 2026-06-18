#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace hyh {

    struct Lexeme {
        enum class Kind {
            HebrewWord,
            AsciiWord,
            Number,
            String,
            Comma
        };

        Kind kind = Kind::HebrewWord;
        std::string text;
        int intValue = 0;
        std::size_t line = 1;
        std::size_t column = 1;
    };

    enum class RawLineTerminator {
        Newline,
        Dot,
        Question,
        Colon,
        Eof
    };

    struct RawLine {
        std::vector<Lexeme> lexemes;
        RawLineTerminator terminator = RawLineTerminator::Newline;
        std::size_t line = 1;
        std::size_t indent = 0;

        [[nodiscard]] bool startsBlock() const {
            return terminator == RawLineTerminator::Colon;
        }
    };

    struct RawNode {
        RawLine line;
        std::vector<RawNode> children;
    };

    struct RawDocument {
        std::vector<RawLine> lines;
    };

} // namespace hyh
