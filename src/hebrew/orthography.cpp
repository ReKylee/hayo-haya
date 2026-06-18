#include "hebrew/orthography.hpp"

#include "hebrew/unicode.hpp"

namespace hyh::hebrew {

    OrthographicToken makeOrthographicToken(const Lexeme& lexeme) {
        OrthographicToken token;
        token.surface = lexeme.text;
        token.normalizedSurface = normalizeMaqaf(lexeme.text);
        token.consonantal = stripHebrewMarks(token.normalizedSurface);
        token.hasNiqqud = containsHebrewMarks(token.normalizedSurface);
        token.sourceKind = lexeme.kind;
        token.line = lexeme.line;
        token.column = lexeme.column;

        for (const auto cp: decodeUtf8(token.normalizedSurface)) {
            if (isMaqaf(cp)) {
                token.hasMaqaf = true;
                break;
            }
        }

        return token;
    }

} // namespace hyh::hebrew
