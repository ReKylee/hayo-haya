#pragma once

#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "hebrew/features.hpp"
#include "hebrew/orthography.hpp"
#include "raw/raw_syntax.hpp"

namespace hyh::hebrew {

    struct Morpheme {
        std::string surface;
        std::string lemma;
        MorphemeRole role = MorphemeRole::Unknown;
        std::optional<std::string> root;
        std::optional<std::string> pattern;
        std::optional<HebrewBinyan> binyan;
        HebrewFeatures features;
    };

    struct MorphAnalysis {
        std::string surface;
        std::string consonantal;
        std::string lemma;
        std::optional<std::string> root;
        std::optional<std::string> pattern;
        PartOfSpeech pos = PartOfSpeech::Unknown;
        HebrewFeatures features;
        std::vector<Morpheme> morphemes;
        std::vector<PrefixAnalysis> prefixes;
        std::optional<PossessiveSuffix> possessiveSuffix;
        double score = 0.0;
        AnalysisProvenance provenance = AnalysisProvenance::RawFallback;
        std::vector<std::string> notes;
    };

    struct SurfaceTokenAnalysis {
        OrthographicToken token;
        std::vector<MorphAnalysis> candidates;
    };

    struct MorphLattice {
        std::vector<SurfaceTokenAnalysis> tokens;
    };

    struct LexiconEntry {
        std::string surface;
        std::string lemma;
        PartOfSpeech pos = PartOfSpeech::Unknown;
        HebrewFeatures features;
        std::optional<std::string> root;
        std::optional<std::string> pattern;
        std::optional<std::string> vocalizedSurface;
        double baseScore = 10.0;
    };

    class Analyzer {
    public:
        Analyzer();

        MorphLattice analyze(const std::vector<Lexeme>& lexemes) const;
        SurfaceTokenAnalysis analyzeLexeme(const Lexeme& lexeme) const;
        std::vector<MorphAnalysis> analyzeToken(const OrthographicToken& token) const;

    private:
        void addEntry(LexiconEntry entry);
        std::vector<MorphAnalysis> lexiconCandidates(const OrthographicToken& token) const;
        std::vector<MorphAnalysis> prefixCandidates(const OrthographicToken& token) const;
        std::vector<MorphAnalysis> possessiveCandidates(const OrthographicToken& token) const;
        std::vector<MorphAnalysis> maqafCandidates(const OrthographicToken& token) const;
        std::vector<MorphAnalysis> oovCandidates(const OrthographicToken& token) const;
        std::vector<MorphAnalysis> applyNiqqudPolicy(const OrthographicToken& token,
                                                     std::vector<MorphAnalysis> candidates) const;

        std::unordered_multimap<std::string, LexiconEntry> lexicon_;
    };

    const MorphAnalysis* bestCandidate(const SurfaceTokenAnalysis& token);
    const MorphAnalysis* bestCandidateWithPos(const SurfaceTokenAnalysis& token, PartOfSpeech pos);
    void dumpLattice(std::ostream& out, const MorphLattice& lattice);

} // namespace hyh::hebrew
