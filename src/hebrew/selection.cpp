#include "hebrew/selection.hpp"

namespace hyh::hebrew {
    namespace {

        bool posMatches(const MorphAnalysis& analysis, ExpectedCategory category) {
            switch (category) {
            case ExpectedCategory::Any:
                return true;
            case ExpectedCategory::Name:
                return analysis.pos == PartOfSpeech::Name;
            case ExpectedCategory::Noun:
                return analysis.pos == PartOfSpeech::Noun;
            case ExpectedCategory::RoleNoun:
                return analysis.pos == PartOfSpeech::Noun && analysis.features.isRoleCandidate;
            case ExpectedCategory::Verb:
                return analysis.pos == PartOfSpeech::Verb;
            case ExpectedCategory::SpeechVerb:
                return analysis.pos == PartOfSpeech::Verb
                    && (analysis.lemma == "קרא" || analysis.lemma == "אמר" || analysis.lemma == "לחש");
            case ExpectedCategory::PropertyNoun:
                return analysis.pos == PartOfSpeech::Noun && analysis.features.isPropertyCandidate;
            case ExpectedCategory::ObjectNoun:
                return analysis.pos == PartOfSpeech::Noun && analysis.features.isObjectCandidate;
            case ExpectedCategory::Quantifier:
                return analysis.pos == PartOfSpeech::Quantifier;
            case ExpectedCategory::Preposition:
                return analysis.pos == PartOfSpeech::Preposition;
            case ExpectedCategory::Number:
                return analysis.pos == PartOfSpeech::Number;
            case ExpectedCategory::StringLiteral:
                return analysis.pos == PartOfSpeech::StringLiteral;
            }
            return false;
        }

        bool featureMatches(const MorphAnalysis& analysis, const GrammarExpectation& expectation) {
            if (expectation.gender && analysis.features.gender != Gender::Unknown
                && analysis.features.gender != *expectation.gender) {
                return false;
            }
            if (expectation.number && analysis.features.number != Number::Unknown
                && analysis.features.number != *expectation.number) {
                return false;
            }
            if (expectation.tense && analysis.features.tenseAspect != TenseAspect::Unknown
                && analysis.features.tenseAspect != *expectation.tense) {
                return false;
            }
            if (expectation.binyan && analysis.features.binyan != HebrewBinyan::Unknown
                && analysis.features.binyan != *expectation.binyan) {
                return false;
            }
            if (!expectation.allowConstruct && analysis.features.state == State::Construct) {
                return false;
            }
            if (!expectation.allowName && analysis.pos == PartOfSpeech::Name) {
                return false;
            }
            return true;
        }

    } // namespace

    SelectionResult SelectionEngine::select(const SurfaceTokenAnalysis& token,
                                            const GrammarExpectation& expectation) const {
        SelectionResult result;
        double bestScore = -1.0;
        for (const auto& candidate: token.candidates) {
            if (!posMatches(candidate, expectation.category) || !featureMatches(candidate, expectation)) {
                continue;
            }
            if (candidate.score > bestScore) {
                bestScore = candidate.score;
                result.analysis = &candidate;
                result.forcedByExpectation = expectation.category != ExpectedCategory::Any;
                result.ambiguous = false;
            } else if (candidate.score == bestScore) {
                result.ambiguous = true;
            }
        }
        if (!result.analysis && !token.candidates.empty()) {
            result.analysis = &token.candidates.front();
            result.ambiguous = token.candidates.size() > 1;
        }
        return result;
    }

} // namespace hyh::hebrew
