#pragma once

#include <string>

namespace hyh::hebrew {

    enum class PartOfSpeech {
        Unknown,
        Name,
        Noun,
        Verb,
        Adjective,
        Adverb,
        Preposition,
        Conjunction,
        Determiner,
        Quantifier,
        Number,
        StringLiteral,
        FunctionWord
    };

    enum class Gender {
        Unknown,
        Masculine,
        Feminine,
        Common
    };

    enum class Number {
        Unknown,
        Singular,
        Plural,
        Dual
    };

    enum class Person {
        Unknown,
        First,
        Second,
        Third
    };

    enum class Definiteness {
        Unknown,
        Indefinite,
        Definite,
        Construct
    };

    enum class State {
        Unknown,
        Absolute,
        Construct
    };

    enum class TenseAspect {
        Unknown,
        Past,
        Present,
        Future,
        Imperative,
        Infinitive
    };

    enum class HebrewBinyan {
        Unknown,
        Paal,
        Nifal,
        Piel,
        Pual,
        Hifil,
        Hufal,
        Hitpael,
        Nitpael
    };

    enum class MorphemeRole {
        Unknown,
        Prefix,
        ConjunctionPrefix,
        DefiniteArticle,
        PrepositionPrefix,
        ComparisonPrefix,
        RootStem,
        Noun,
        Verb,
        Adjective,
        Name,
        PossessiveSuffix,
        StringLiteral,
        NumberLiteral
    };

    enum class AnalysisProvenance {
        BuiltinLexicon,
        StoryLexicon,
        RuleDerived,
        NiqqudFiltered,
        ParserForced,
        AuthorForced,
        OovGuess,
        RawFallback
    };

    enum class PrefixKind {
        Conjunction,
        DefiniteArticle,
        In,
        To,
        From,
        As,
        Unknown
    };

    struct PrefixAnalysis {
        std::string surface;
        PrefixKind kind = PrefixKind::Unknown;
    };

    struct PossessiveSuffix {
        std::string surface;
        Person person = Person::Unknown;
        Gender gender = Gender::Unknown;
        Number number = Number::Unknown;
    };

    struct HebrewFeatures {
        Gender gender = Gender::Unknown;
        Number number = Number::Unknown;
        Person person = Person::Unknown;
        Definiteness definiteness = Definiteness::Unknown;
        State state = State::Unknown;
        TenseAspect tenseAspect = TenseAspect::Unknown;
        HebrewBinyan binyan = HebrewBinyan::Unknown;
        bool isRoleCandidate = false;
        bool isObjectCandidate = false;
        bool isPropertyCandidate = false;
        bool isNameCandidate = false;
        bool isAnimateCandidate = false;
    };

    std::string toString(PartOfSpeech value);
    std::string toString(Gender value);
    std::string toString(Number value);
    std::string toString(Person value);
    std::string toString(Definiteness value);
    std::string toString(State value);
    std::string toString(TenseAspect value);
    std::string toString(HebrewBinyan value);
    std::string toString(AnalysisProvenance value);

} // namespace hyh::hebrew
