#pragma once

#include <optional>
#include <string>

#include "fairy/story_ast.hpp"
#include "hebrew/sentence_ir.hpp"

namespace hyh::fairy {

    struct MirrorQuestion {
        std::string prompt;
        std::string slotName;
        AnswerType answerTypeHint = AnswerType::Unknown;
    };

    std::optional<MirrorQuestion> recognizeMirrorQuestion(const hyh::hebrew::HebrewSentence& sentence);

} // namespace hyh::fairy
