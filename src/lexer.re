#include "lexer.hpp"

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace hyh {

Lexer::Lexer(std::string input)
    : input_(std::move(input)) {
    input_.push_back('\0');
    cursor_ = input_.data();
    marker_ = cursor_;
}

static std::string slice(const char* begin, const char* end) {
    return std::string(begin, end);
}

static std::string stripHebrewMarks(const std::string& text) {
    std::string result;
    for (std::size_t i = 0; i < text.size();) {
        const auto first = static_cast<unsigned char>(text[i]);
        if (i + 1 < text.size()) {
            const auto second = static_cast<unsigned char>(text[i + 1]);
            const bool isHebrewMark =
                (first == 0xD6 && second >= 0x91 && second <= 0xBF && second != 0xBE)
                || (first == 0xD7 && second >= 0x80 && second <= 0x87);
            if (isHebrewMark) {
                i += 2;
                continue;
            }
        }

        result.push_back(text[i]);
        ++i;
    }
    return result;
}

Parser::symbol_type Lexer::next() {
    const char* token_start = cursor_;

    /*!re2c
        re2c:yyfill:enable = 0;
        re2c:encoding:utf8 = 1;
        re2c:define:YYCTYPE = "unsigned char";
        re2c:define:YYCURSOR = cursor_;
        re2c:define:YYMARKER = marker_;

        HebrewLetter = [א-ת];
        HebrewMark   = [\xD6][\x91-\xBD] | [\xD6][\xBF] | [\xD7][\x80-\x87];
        HebrewUnit   = HebrewLetter HebrewMark*;
        HebrewWord   = HebrewUnit+ ("־" HebrewUnit+)*;
        AsciiName    = [A-Za-z_][A-Za-z0-9_]*;
        Digit        = [0-9];
        Number       = Digit+;

        [ \t\r\n]+ {
            return next();
        }

        "\x00" {
            return Parser::make_EOF_TOKEN(location_);
        }

        "." { return Parser::make_DOT(location_); }
        "," { return Parser::make_COMMA(location_); }
        ":" { return Parser::make_COLON(location_); }
        "מ־" { return Parser::make_MI_PREFIX(location_); }
        "הָיֹה" { return Parser::make_HAYA(location_); }
        "הָיְתָה" { return Parser::make_HAYTA(location_); }

        "\"" ([^"\\] | "\\" .)* "\"" {
            std::string raw = slice(token_start + 1, cursor_ - 1);
            return Parser::make_STRING(raw, location_);
        }

        Number {
            int value = std::stoi(slice(token_start, cursor_));
            return Parser::make_NUMBER(value, location_);
        }

        AsciiName {
            return Parser::make_ASCII_NAME(slice(token_start, cursor_), location_);
        }

        HebrewWord {
            return classifyHebrewWord(token_start, cursor_);
        }

        * {
            throw std::runtime_error("Unexpected character in lexer near byte: " + slice(token_start, cursor_));
        }
    */
}

Parser::symbol_type Lexer::classifyHebrewWord(const char* begin, const char* end) {
    const std::string text = stripHebrewMarks(slice(begin, end));

    if (text == "היה") return Parser::make_HAYA(location_);
    if (text == "הייתה") return Parser::make_HAYTA(location_);
    if (text == "בארץ") return Parser::make_BEERETZ(location_);
    if (text == "רחוקה") return Parser::make_REHOKA(location_);
    if (text == "ושמה") return Parser::make_VESHMA(location_);
    if (text == "ממלכה") return Parser::make_MAMLAKHA(location_);
    if (text == "קטנה") return Parser::make_KETANA(location_);
    if (text == "וכך") return Parser::make_VEKAKH(location_);
    if (text == "תם") return Parser::make_TAM(location_);
    if (text == "סיפורה") return Parser::make_SIPURA(location_);
    if (text == "של") return Parser::make_SHEL(location_);
    if (text == "ממלכת") return Parser::make_MAMLEKHET(location_);

    if (text == "מן") return Parser::make_MIN(location_);
    if (text == "הממלכה") return Parser::make_HAMAMLAKHA(location_);
    if (text == "העתיקה") return Parser::make_HAATIKA(location_);
    if (text == "הגיע") return Parser::make_HIGIA(location_);
    if (text == "ושמו") return Parser::make_USHMO(location_);

    if (text == "על") return Parser::make_AL(location_);
    if (text == "אל") return Parser::make_EL(location_);
    if (text == "נכתב") return Parser::make_NICHTAV(location_);
    if (text == "נחו") return Parser::make_NACHU(location_);
    if (text == "קרא") return Parser::make_KARA(location_);
    if (text == "את") return Parser::make_ET(location_);
    if (text == "הכתוב") return Parser::make_HAKATUV(location_);
    if (text == "שעל") return Parser::make_SHEAL(location_);
    if (text == "נפתח") return Parser::make_NIFTACH(location_);
    if (text == "נוסף") return Parser::make_NOSAF(location_);
    if (text == "שוב") return Parser::make_SHUV(location_);
    if (text == "ושוב") return Parser::make_VESHUV(location_);
    if (text == "כל") return Parser::make_KOL(location_);
    if (text == "עוד") return Parser::make_OD(location_);
    if (text == "פחות") return Parser::make_PACHOT(location_);
    if (text == "כך") return Parser::make_KAKH(location_);
    if (text == "חזר") return Parser::make_CHAZAR(location_);
    if (text == "הדבר") return Parser::make_HADAVAR(location_);
    if (text == "כאשר") return Parser::make_KAASHER(location_);
    if (text == "אמרה") return Parser::make_AMRA(location_);
    if (text == "ידעה") return Parser::make_YADAA(location_);
    if (text == "כי") return Parser::make_KI(location_);
    if (text == "אמת") return Parser::make_EMET(location_);
    if (text == "בפי") return Parser::make_BEFI(location_);
    if (text == "נודע") return Parser::make_NODA(location_);
    if (text == "דבר") return Parser::make_DAVAR(location_);
    if (text == "בכל") return Parser::make_BEKHOL(location_);
    if (text == "הביטה") return Parser::make_HIBITA(location_);
    if (text == "ושתקה") return Parser::make_VESHAKTA(location_);
    if (text == "הסתירה") return Parser::make_HISTIRA(location_);
    if (text == "מתחת") return Parser::make_MITACHAT(location_);
    if (text == "שאלה") return Parser::make_SHAALA(location_);

    if (text == "אפס") return Parser::make_NUMBER(0, location_);
    if (text == "אחד" || text == "אחת") return Parser::make_NUMBER(1, location_);
    if (text == "שניים" || text == "שתיים") return Parser::make_NUMBER(2, location_);
    if (text == "שלושה" || text == "שלוש") return Parser::make_NUMBER(3, location_);
    if (text == "ארבעה" || text == "ארבע") return Parser::make_NUMBER(4, location_);
    if (text == "חמישה" || text == "חמש") return Parser::make_NUMBER(5, location_);
    if (text == "עשרים") return Parser::make_NUMBER(20, location_);

    return Parser::make_NAME(text, location_);
}

} // namespace hyh
