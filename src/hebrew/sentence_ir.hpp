#pragma once

#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "hebrew/morphology.hpp"

namespace hyh::hebrew {

    enum class ConstructRelation {
        Unknown,
        Possession,
        Material,
        Type,
        Measure,
        LexicalizedCompound
    };

    enum class HebrewSentenceKind {
        Unknown,
        RoleIntroduction,
        PropertyAssignment,
        ForEachRole,
        Condition,
        Speech,
        Import,
        TeachingDirective,
        NarrativeEvent
    };

    struct NounPhrase {
        std::string surface;
        std::string headLemma;
        Gender gender = Gender::Unknown;
        Number number = Number::Unknown;
        Definiteness definiteness = Definiteness::Unknown;
        bool isConstruct = false;
        bool isAnimateCandidate = false;
        std::vector<std::string> parts;
        std::optional<PossessiveSuffix> possessive;
        ConstructRelation relation = ConstructRelation::Unknown;
    };

    struct VerbReading {
        std::string surface;
        std::string lemma;
        std::optional<std::string> root;
        HebrewBinyan binyan = HebrewBinyan::Unknown;
        Gender gender = Gender::Unknown;
        Number number = Number::Unknown;
        Person person = Person::Unknown;
        TenseAspect tenseAspect = TenseAspect::Unknown;
    };

    struct NumberExpressionIr {
        enum class Kind {
            Unknown,
            Literal,
            CurrentActorProperty,
            EntityProperty
        };

        Kind kind = Kind::Unknown;
        int literal = 0;
        std::string entityName;
        std::string entitySurface;
        std::string entityHeadLemma;
        bool entityDefinite = false;
        Gender entityGender = Gender::Unknown;
        Number entityNumber = Number::Unknown;
        std::string propertyName;
        std::string rawSurface;
        std::vector<std::string> phraseLemmas;
        bool comparisonPrefixed = false;
    };

    struct HebrewSentence {
        HebrewSentenceKind kind = HebrewSentenceKind::Unknown;
        std::size_t line = 1;
        std::string rawText;
        std::vector<SurfaceTokenAnalysis> tokens;

        std::string subjectName;
        NounPhrase role;
        std::vector<std::string> attributes;

        VerbReading predicate;
        NounPhrase object;
        NumberExpressionIr leftNumber;
        NumberExpressionIr rightNumber;
        int literalValue = 0;
        std::string speechText;
        std::string speakerName;
        std::string importedName;
        std::string importAlias;
        std::vector<std::string> diagnostics;
    };

    class SentenceAnalyzer {
    public:
        HebrewSentence analyze(const MorphLattice& lattice, const RawLine& line) const;

    private:
        HebrewSentence tryRoleIntroduction(const MorphLattice& lattice, const RawLine& line) const;
        HebrewSentence tryPropertyAssignment(const MorphLattice& lattice, const RawLine& line) const;
        HebrewSentence tryForEachRole(const MorphLattice& lattice, const RawLine& line) const;
        HebrewSentence tryCondition(const MorphLattice& lattice, const RawLine& line) const;
        HebrewSentence tryImport(const MorphLattice& lattice, const RawLine& line) const;
        HebrewSentence trySpeech(const MorphLattice& lattice, const RawLine& line) const;
        HebrewSentence narrativeEvent(const MorphLattice& lattice, const RawLine& line) const;
    };

    void dumpSentenceIr(std::ostream& out, const HebrewSentence& sentence);

} // namespace hyh::hebrew
