#include "hebrew/unicode.hpp"

#include <stdexcept>

namespace hyh::hebrew {

    std::vector<char32_t> decodeUtf8(const std::string& text) {
        std::vector<char32_t> out;
        for (std::size_t i = 0; i < text.size();) {
            const auto c = static_cast<unsigned char>(text[i]);
            if (c < 0x80) {
                out.push_back(c);
                ++i;
            } else if ((c >> 5) == 0b110 && i + 1 < text.size()) {
                const char32_t cp = ((c & 0x1F) << 6) | (static_cast<unsigned char>(text[i + 1]) & 0x3F);
                out.push_back(cp);
                i += 2;
            } else if ((c >> 4) == 0b1110 && i + 2 < text.size()) {
                const char32_t cp = ((c & 0x0F) << 12) | ((static_cast<unsigned char>(text[i + 1]) & 0x3F) << 6)
                                  | (static_cast<unsigned char>(text[i + 2]) & 0x3F);
                out.push_back(cp);
                i += 3;
            } else if ((c >> 3) == 0b11110 && i + 3 < text.size()) {
                const char32_t cp = ((c & 0x07) << 18) | ((static_cast<unsigned char>(text[i + 1]) & 0x3F) << 12)
                                  | ((static_cast<unsigned char>(text[i + 2]) & 0x3F) << 6)
                                  | (static_cast<unsigned char>(text[i + 3]) & 0x3F);
                out.push_back(cp);
                i += 4;
            } else {
                throw std::runtime_error("invalid UTF-8 in source");
            }
        }
        return out;
    }

    std::string encodeUtf8(char32_t cp) {
        std::string out;
        if (cp <= 0x7F) {
            out.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        return out;
    }

    std::string encodeUtf8(const std::vector<char32_t>& codepoints) {
        std::string out;
        for (const auto cp: codepoints) {
            out += encodeUtf8(cp);
        }
        return out;
    }

    bool isHebrewLetter(char32_t cp) {
        return cp >= 0x05D0 && cp <= 0x05EA;
    }

    bool isHebrewMark(char32_t cp) {
        return (cp >= 0x0591 && cp <= 0x05BD) || cp == 0x05BF || (cp >= 0x05C1 && cp <= 0x05C2)
            || (cp >= 0x05C4 && cp <= 0x05C5) || cp == 0x05C7;
    }

    bool isHebrewCodepoint(char32_t cp) {
        return cp >= 0x0590 && cp <= 0x05FF;
    }

    bool isMaqaf(char32_t cp) {
        return cp == 0x05BE;
    }

    std::string stripHebrewMarks(const std::string& text) {
        std::vector<char32_t> out;
        for (const auto cp: decodeUtf8(text)) {
            if (!isHebrewMark(cp)) {
                out.push_back(cp);
            }
        }
        return encodeUtf8(out);
    }

    bool containsHebrewMarks(const std::string& text) {
        for (const auto cp: decodeUtf8(text)) {
            if (isHebrewMark(cp)) {
                return true;
            }
        }
        return false;
    }

    std::string normalizeMaqaf(const std::string& text) {
        std::vector<char32_t> out;
        for (const auto cp: decodeUtf8(text)) {
            out.push_back(cp == '-' ? 0x05BE : cp);
        }
        return encodeUtf8(out);
    }

} // namespace hyh::hebrew
