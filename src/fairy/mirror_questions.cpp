#include "fairy/mirror_questions.hpp"

#include <algorithm>
#include <sstream>
#include <string_view>
#include <vector>

namespace hyh::fairy {
    namespace {

        std::string stripDefiniteArticle(std::string value) {
            if (value.starts_with("ה") && value.size() > std::string("ה").size()) {
                return value.substr(std::string("ה").size());
            }
            return value;
        }

        bool tokenIs(const hyh::hebrew::SurfaceTokenAnalysis& token, std::string_view value) {
            return token.token.consonantal == value;
        }

        bool tokenContains(const hyh::hebrew::SurfaceTokenAnalysis& token, std::string_view value) {
            return token.token.consonantal.find(value) != std::string::npos;
        }

        bool isMirrorToken(const hyh::hebrew::SurfaceTokenAnalysis& token) {
            const auto value = stripDefiniteArticle(token.token.consonantal);
            return value == "מראה" || value == "מראת" || value.starts_with("מראת־") || value.starts_with("מראה־");
        }

        bool isMagicToken(const hyh::hebrew::SurfaceTokenAnalysis& token) {
            return stripDefiniteArticle(token.token.consonantal) == "קסם" || tokenContains(token, "קסם");
        }

        bool isQuestionWord(const hyh::hebrew::SurfaceTokenAnalysis& token) {
            return tokenIs(token, "מי") || tokenIs(token, "מה") || tokenIs(token, "כמה") || tokenIs(token, "למה")
                || tokenIs(token, "איפה") || tokenIs(token, "היכן") || tokenIs(token, "האם");
        }

        bool hasAskVerb(const hyh::hebrew::HebrewSentence& sentence) {
            return std::ranges::any_of(sentence.tokens, [](const auto& token) {
                return tokenIs(token, "שאל") || tokenIs(token, "שאלה") || tokenIs(token, "שאלו")
                    || tokenIs(token, "שואל") || tokenIs(token, "שואלת") || tokenIs(token, "ביקש")
                    || tokenIs(token, "ביקשה");
            });
        }

        bool hasMagicMirrorReference(const hyh::hebrew::HebrewSentence& sentence) {
            for (std::size_t i = 0; i < sentence.tokens.size(); ++i) {
                const auto& token = sentence.tokens[i];
                if (!isMirrorToken(token)) {
                    continue;
                }
                if (isMagicToken(token)) {
                    return true;
                }
                if (i + 1 < sentence.tokens.size() && isMagicToken(sentence.tokens[i + 1])) {
                    return true;
                }
            }
            return false;
        }

        bool startsWithMirrorIncantation(const hyh::hebrew::HebrewSentence& sentence) {
            return sentence.tokens.size() >= 2 && isMirrorToken(sentence.tokens[0])
                && isMirrorToken(sentence.tokens[1]);
        }

        std::string trim(std::string value) {
            constexpr std::string_view whitespace = " \t\r\n";
            const auto first = value.find_first_not_of(whitespace);
            if (first == std::string::npos) {
                return {};
            }
            const auto last = value.find_last_not_of(whitespace);
            return value.substr(first, last - first + 1);
        }

        std::string firstQuotedString(const hyh::hebrew::HebrewSentence& sentence) {
            for (const auto& token: sentence.tokens) {
                if (token.token.sourceKind == Lexeme::Kind::String) {
                    return token.token.surface;
                }
            }
            return {};
        }

        std::string joinTokenSurfaces(const hyh::hebrew::HebrewSentence& sentence, std::size_t begin) {
            std::string out;
            for (std::size_t i = begin; i < sentence.tokens.size(); ++i) {
                if (sentence.tokens[i].token.sourceKind == Lexeme::Kind::String) {
                    continue;
                }
                if (!out.empty()) {
                    out += ' ';
                }
                out += sentence.tokens[i].token.surface;
            }
            return out;
        }

        std::vector<std::string> promptWords(std::string prompt) {
            for (char& c: prompt) {
                if (c == '?' || c == '.' || c == ',' || c == ':' || c == '"' || c == '\'') {
                    c = ' ';
                }
            }

            std::vector<std::string> words;
            std::istringstream in(prompt);
            for (std::string word; in >> word;) {
                words.push_back(std::move(word));
            }
            return words;
        }

        std::string joinWords(const std::vector<std::string>& words, std::size_t begin) {
            std::string out;
            for (std::size_t i = begin; i < words.size(); ++i) {
                if (!out.empty()) {
                    out += ' ';
                }
                out += words[i];
            }
            return out;
        }

        std::string withDefiniteArticle(const std::string& value) {
            if (value.empty() || value.starts_with("ה")) {
                return value;
            }
            return "ה" + value;
        }

        AnswerType answerTypeHintFromQuestion(const std::vector<std::string>& words) {
            if (words.empty()) {
                return AnswerType::Unknown;
            }
            const auto& word = words.front();
            if (word == "כמה") {
                return AnswerType::Number;
            }
            if (word == "האם") {
                return AnswerType::Boolean;
            }
            return AnswerType::Unknown;
        }

        std::string slotFromQuestionWords(const std::vector<std::string>& words) {
            if (words.empty()) {
                return {};
            }

            const auto& questionWord = words.front();
            if (questionWord == "מי") {
                std::size_t predicateStart = 1;
                if (predicateStart < words.size() && words[predicateStart] == "הכי") {
                    ++predicateStart;
                }
                if (predicateStart >= words.size()) {
                    return {};
                }

                std::vector<std::string> slotWords;
                slotWords.push_back(withDefiniteArticle(words[predicateStart]));
                slotWords.insert(slotWords.end(), words.begin() + static_cast<long>(predicateStart + 1), words.end());
                return joinWords(slotWords, 0);
            }

            if (questionWord == "מה" || questionWord == "כמה" || questionWord == "למה" || questionWord == "איפה"
                || questionWord == "היכן" || questionWord == "האם") {
                return joinWords(words, 1);
            }

            return joinWords(words, 0);
        }

        std::string mirrorIncantationPrompt(const hyh::hebrew::HebrewSentence& sentence) {
            for (std::size_t i = 0; i < sentence.tokens.size(); ++i) {
                if (isQuestionWord(sentence.tokens[i])) {
                    return joinTokenSurfaces(sentence, i);
                }
            }

            const auto comma = sentence.rawText.find(',');
            if (comma != std::string::npos) {
                return trim(sentence.rawText.substr(comma + 1));
            }
            return sentence.rawText;
        }

    } // namespace

    std::optional<MirrorQuestion> recognizeMirrorQuestion(const hyh::hebrew::HebrewSentence& sentence) {
        std::string prompt;
        if (startsWithMirrorIncantation(sentence)) {
            prompt = mirrorIncantationPrompt(sentence);
        } else if (hasAskVerb(sentence) && hasMagicMirrorReference(sentence)) {
            prompt = firstQuotedString(sentence);
        } else {
            return std::nullopt;
        }

        auto words = promptWords(prompt);
        if (words.empty()) {
            return std::nullopt;
        }

        auto slot = slotFromQuestionWords(words);
        if (slot.empty()) {
            return std::nullopt;
        }

        return MirrorQuestion{prompt, slot, answerTypeHintFromQuestion(words)};
    }

} // namespace hyh::fairy
