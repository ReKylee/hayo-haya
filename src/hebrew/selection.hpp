#pragma once

#include <optional>

#include "hebrew/morphology.hpp"

namespace hyh::hebrew {

    enum class ExpectedCategory {
        Any,
        Name,
        Noun,
        RoleNoun,
        Verb,
        SpeechVerb,
        PropertyNoun,
        ObjectNoun,
        Quantifier,
        Preposition,
        Number,
        StringLiteral
    };

    struct GrammarExpectation {
        ExpectedCategory category = ExpectedCategory::Any;
        std::optional<Gender> gender;
        std::optional<Number> number;
        std::optional<TenseAspect> tense;
        std::optional<HebrewBinyan> binyan;
        bool allowConstruct = true;
        bool allowName = true;
    };

    struct SelectionResult {
        const MorphAnalysis* analysis = nullptr;
        bool forcedByExpectation = false;
        bool ambiguous = false;
    };

    class SelectionEngine {
    public:
        SelectionResult select(const SurfaceTokenAnalysis& token, const GrammarExpectation& expectation) const;
    };

} // namespace hyh::hebrew
