#pragma once

#include <string>
#include <vector>

namespace hyh::hebrew {

    std::vector<char32_t> decodeUtf8(const std::string& text);
    std::string encodeUtf8(const std::vector<char32_t>& codepoints);
    std::string encodeUtf8(char32_t codepoint);

    bool isHebrewLetter(char32_t cp);
    bool isHebrewMark(char32_t cp);
    bool isHebrewCodepoint(char32_t cp);
    bool isMaqaf(char32_t cp);

    std::string stripHebrewMarks(const std::string& text);
    bool containsHebrewMarks(const std::string& text);
    std::string normalizeMaqaf(const std::string& text);

} // namespace hyh::hebrew
