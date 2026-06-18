#pragma once

#include <string>

#include "raw/raw_syntax.hpp"

namespace hyh::hebrew {

    struct OrthographicToken {
        std::string surface;
        std::string normalizedSurface;
        std::string consonantal;
        bool hasNiqqud = false;
        bool hasMaqaf = false;
        Lexeme::Kind sourceKind = Lexeme::Kind::HebrewWord;
        std::size_t line = 1;
        std::size_t column = 1;
    };

    OrthographicToken makeOrthographicToken(const Lexeme& lexeme);

} // namespace hyh::hebrew
