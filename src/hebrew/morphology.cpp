#include "hebrew/morphology.hpp"

#include "hebrew/unicode.hpp"

#include <algorithm>
#include <iomanip>

namespace hyh::hebrew {
    namespace {

        HebrewFeatures features(Gender gender = Gender::Unknown, Number number = Number::Unknown) {
            HebrewFeatures result;
            result.gender = gender;
            result.number = number;
            return result;
        }

        LexiconEntry noun(std::string surface, std::string lemma, Gender gender, Number number, double score = 20.0,
                          bool animate = false) {
            LexiconEntry entry;
            entry.surface = std::move(surface);
            entry.lemma = std::move(lemma);
            entry.pos = PartOfSpeech::Noun;
            entry.features = features(gender, number);
            entry.features.isRoleCandidate = true;
            entry.features.isObjectCandidate = true;
            entry.features.isPropertyCandidate = true;
            entry.features.isAnimateCandidate = animate;
            entry.baseScore = score;
            return entry;
        }

        LexiconEntry adjective(std::string surface, std::string lemma, Gender gender, Number number) {
            LexiconEntry entry;
            entry.surface = std::move(surface);
            entry.lemma = std::move(lemma);
            entry.pos = PartOfSpeech::Adjective;
            entry.features = features(gender, number);
            entry.baseScore = 16.0;
            return entry;
        }

        LexiconEntry functionWord(std::string surface, std::string lemma, PartOfSpeech pos) {
            LexiconEntry entry;
            entry.surface = std::move(surface);
            entry.lemma = std::move(lemma);
            entry.pos = pos;
            entry.features = HebrewFeatures{};
            entry.baseScore = 25.0;
            return entry;
        }

        LexiconEntry numberWord(std::string surface, std::string lemma) {
            LexiconEntry entry;
            entry.surface = std::move(surface);
            entry.lemma = std::move(lemma);
            entry.pos = PartOfSpeech::Number;
            entry.baseScore = 30.0;
            return entry;
        }

        LexiconEntry verb(std::string surface, std::string lemma, std::string root, HebrewBinyan binyan, Gender gender,
                          Number number, TenseAspect tense, double score = 22.0) {
            LexiconEntry entry;
            entry.surface = std::move(surface);
            entry.lemma = std::move(lemma);
            entry.root = std::move(root);
            entry.pos = PartOfSpeech::Verb;
            entry.features = features(gender, number);
            entry.features.person = Person::Third;
            entry.features.tenseAspect = tense;
            entry.features.binyan = binyan;
            entry.baseScore = score;
            return entry;
        }

        Morpheme stemMorpheme(const LexiconEntry& entry) {
            Morpheme morpheme;
            morpheme.surface = entry.surface;
            morpheme.lemma = entry.lemma;
            morpheme.role = entry.pos == PartOfSpeech::Verb ? MorphemeRole::Verb : MorphemeRole::RootStem;
            morpheme.root = entry.root;
            morpheme.pattern = entry.pattern;
            morpheme.binyan =
                entry.features.binyan == HebrewBinyan::Unknown ? std::optional<HebrewBinyan>{} : entry.features.binyan;
            morpheme.features = entry.features;
            return morpheme;
        }

        MorphAnalysis fromEntry(const OrthographicToken& token, const LexiconEntry& entry) {
            MorphAnalysis analysis;
            analysis.surface = token.surface;
            analysis.consonantal = token.consonantal;
            analysis.lemma = entry.lemma;
            analysis.root = entry.root;
            analysis.pattern = entry.pattern;
            analysis.pos = entry.pos;
            analysis.features = entry.features;
            analysis.score = entry.baseScore;
            analysis.provenance = AnalysisProvenance::BuiltinLexicon;
            analysis.morphemes.push_back(stemMorpheme(entry));
            analysis.notes.push_back("builtin lexicon candidate");
            return analysis;
        }

        std::optional<PrefixAnalysis> prefixFor(char32_t cp) {
            if (cp == 0x05D5) {
                return PrefixAnalysis{"ו", PrefixKind::Conjunction};
            }
            if (cp == 0x05D4) {
                return PrefixAnalysis{"ה", PrefixKind::DefiniteArticle};
            }
            if (cp == 0x05D1) {
                return PrefixAnalysis{"ב", PrefixKind::In};
            }
            if (cp == 0x05DC) {
                return PrefixAnalysis{"ל", PrefixKind::To};
            }
            if (cp == 0x05DE) {
                return PrefixAnalysis{"מ", PrefixKind::From};
            }
            if (cp == 0x05DB) {
                return PrefixAnalysis{"כ", PrefixKind::As};
            }
            return std::nullopt;
        }

        Morpheme prefixMorpheme(const PrefixAnalysis& prefix) {
            Morpheme morpheme;
            morpheme.surface = prefix.surface;
            morpheme.lemma = prefix.surface;
            switch (prefix.kind) {
            case PrefixKind::Conjunction:
                morpheme.role = MorphemeRole::ConjunctionPrefix;
                break;
            case PrefixKind::DefiniteArticle:
                morpheme.role = MorphemeRole::DefiniteArticle;
                break;
            case PrefixKind::As:
                morpheme.role = MorphemeRole::ComparisonPrefix;
                break;
            default:
                morpheme.role = MorphemeRole::PrepositionPrefix;
                break;
            }
            return morpheme;
        }

        std::string joinSurface(const std::vector<std::string>& parts, const std::string& sep) {
            std::string out;
            for (std::size_t i = 0; i < parts.size(); ++i) {
                if (i > 0) {
                    out += sep;
                }
                out += parts[i];
            }
            return out;
        }

    } // namespace

    std::string toString(PartOfSpeech value) {
        switch (value) {
        case PartOfSpeech::Name:
            return "Name";
        case PartOfSpeech::Noun:
            return "Noun";
        case PartOfSpeech::Verb:
            return "Verb";
        case PartOfSpeech::Adjective:
            return "Adjective";
        case PartOfSpeech::Adverb:
            return "Adverb";
        case PartOfSpeech::Preposition:
            return "Preposition";
        case PartOfSpeech::Conjunction:
            return "Conjunction";
        case PartOfSpeech::Determiner:
            return "Determiner";
        case PartOfSpeech::Quantifier:
            return "Quantifier";
        case PartOfSpeech::Number:
            return "Number";
        case PartOfSpeech::StringLiteral:
            return "String";
        case PartOfSpeech::FunctionWord:
            return "FunctionWord";
        case PartOfSpeech::Unknown:
            return "Unknown";
        }
        return "Unknown";
    }

    std::string toString(Gender value) {
        switch (value) {
        case Gender::Masculine:
            return "Masculine";
        case Gender::Feminine:
            return "Feminine";
        case Gender::Common:
            return "Common";
        case Gender::Unknown:
            return "Unknown";
        }
        return "Unknown";
    }

    std::string toString(Number value) {
        switch (value) {
        case Number::Singular:
            return "Singular";
        case Number::Plural:
            return "Plural";
        case Number::Dual:
            return "Dual";
        case Number::Unknown:
            return "Unknown";
        }
        return "Unknown";
    }

    std::string toString(Person value) {
        switch (value) {
        case Person::First:
            return "First";
        case Person::Second:
            return "Second";
        case Person::Third:
            return "Third";
        case Person::Unknown:
            return "Unknown";
        }
        return "Unknown";
    }

    std::string toString(Definiteness value) {
        switch (value) {
        case Definiteness::Indefinite:
            return "Indefinite";
        case Definiteness::Definite:
            return "Definite";
        case Definiteness::Construct:
            return "Construct";
        case Definiteness::Unknown:
            return "Unknown";
        }
        return "Unknown";
    }

    std::string toString(State value) {
        switch (value) {
        case State::Absolute:
            return "Absolute";
        case State::Construct:
            return "Construct";
        case State::Unknown:
            return "Unknown";
        }
        return "Unknown";
    }

    std::string toString(TenseAspect value) {
        switch (value) {
        case TenseAspect::Past:
            return "Past";
        case TenseAspect::Present:
            return "Present";
        case TenseAspect::Future:
            return "Future";
        case TenseAspect::Imperative:
            return "Imperative";
        case TenseAspect::Infinitive:
            return "Infinitive";
        case TenseAspect::Unknown:
            return "Unknown";
        }
        return "Unknown";
    }

    std::string toString(HebrewBinyan value) {
        switch (value) {
        case HebrewBinyan::Paal:
            return "Paal";
        case HebrewBinyan::Nifal:
            return "Nifal";
        case HebrewBinyan::Piel:
            return "Piel";
        case HebrewBinyan::Pual:
            return "Pual";
        case HebrewBinyan::Hifil:
            return "Hifil";
        case HebrewBinyan::Hufal:
            return "Hufal";
        case HebrewBinyan::Hitpael:
            return "Hitpael";
        case HebrewBinyan::Nitpael:
            return "Nitpael";
        case HebrewBinyan::Unknown:
            return "Unknown";
        }
        return "Unknown";
    }

    std::string toString(AnalysisProvenance value) {
        switch (value) {
        case AnalysisProvenance::BuiltinLexicon:
            return "BuiltinLexicon";
        case AnalysisProvenance::StoryLexicon:
            return "StoryLexicon";
        case AnalysisProvenance::RuleDerived:
            return "RuleDerived";
        case AnalysisProvenance::NiqqudFiltered:
            return "NiqqudFiltered";
        case AnalysisProvenance::ParserForced:
            return "ParserForced";
        case AnalysisProvenance::AuthorForced:
            return "AuthorForced";
        case AnalysisProvenance::OovGuess:
            return "OovGuess";
        case AnalysisProvenance::RawFallback:
            return "RawFallback";
        }
        return "Unknown";
    }

    Analyzer::Analyzer() {
        // Closed-class / grammar words.
        addEntry(functionWord("כל", "כל", PartOfSpeech::Quantifier));
        addEntry(functionWord("כאשר", "כאשר", PartOfSpeech::FunctionWord));
        addEntry(functionWord("אל", "אל", PartOfSpeech::Preposition));
        addEntry(functionWord("מן", "מן", PartOfSpeech::Preposition));
        addEntry(functionWord("ושמו", "שם", PartOfSpeech::FunctionWord));
        addEntry(functionWord("שמו", "שם", PartOfSpeech::FunctionWord));
        addEntry(functionWord("בזו", "בזו", PartOfSpeech::Adverb));
        addEntry(functionWord("בזה", "בזה", PartOfSpeech::Adverb));
        addEntry(functionWord("אחר", "אחר", PartOfSpeech::Adverb));
        addEntry(functionWord("כ", "כ", PartOfSpeech::FunctionWord));
        addEntry(functionWord("זו", "זו", PartOfSpeech::FunctionWord));
        addEntry(functionWord("זה", "זה", PartOfSpeech::FunctionWord));
        addEntry(numberWord("אחד", "1"));
        addEntry(numberWord("אחת", "1"));
        addEntry(numberWord("שניים", "2"));
        addEntry(numberWord("שתיים", "2"));
        addEntry(numberWord("שלושה", "3"));
        addEntry(numberWord("שלוש", "3"));
        addEntry(numberWord("ארבעה", "4"));
        addEntry(numberWord("ארבע", "4"));
        addEntry(numberWord("חמישה", "5"));
        addEntry(numberWord("חמש", "5"));
        addEntry(numberWord("עשרים", "20"));

        // Copula / common verbs. These are linguistic facts, not story facts.
        addEntry(verb("היה", "היה", "ה־י־ה", HebrewBinyan::Paal, Gender::Masculine, Number::Singular,
                      TenseAspect::Past));
        addEntry(verb("הייתה", "היה", "ה־י־ה", HebrewBinyan::Paal, Gender::Feminine, Number::Singular,
                      TenseAspect::Past));
        addEntry(verb("היתה", "היה", "ה־י־ה", HebrewBinyan::Paal, Gender::Feminine, Number::Singular,
                      TenseAspect::Past));
        addEntry(verb("היו", "היה", "ה־י־ה", HebrewBinyan::Paal, Gender::Common, Number::Plural, TenseAspect::Past));
        addEntry(verb("ניגשה", "נגש", "נ־ג־ש", HebrewBinyan::Nifal, Gender::Feminine, Number::Singular,
                      TenseAspect::Past));
        addEntry(verb("נִגְּשָׁה", "נגש", "נ־ג־ש", HebrewBinyan::Nifal, Gender::Feminine, Number::Singular,
                      TenseAspect::Past, 30.0));
        addEntry(verb("ניגש", "נגש", "נ־ג־ש", HebrewBinyan::Nifal, Gender::Masculine, Number::Singular,
                      TenseAspect::Past));
        addEntry(verb("ניגשו", "נגש", "נ־ג־ש", HebrewBinyan::Nifal, Gender::Common, Number::Plural, TenseAspect::Past));
        addEntry(verb("קרא", "קרא", "ק־ר־א", HebrewBinyan::Paal, Gender::Masculine, Number::Singular,
                      TenseAspect::Past));
        addEntry(verb("אמר", "אמר", "א־מ־ר", HebrewBinyan::Paal, Gender::Masculine, Number::Singular,
                      TenseAspect::Past));
        addEntry(verb("אמרה", "אמר", "א־מ־ר", HebrewBinyan::Paal, Gender::Feminine, Number::Singular,
                      TenseAspect::Past));
        addEntry(verb("לחשה", "לחש", "ל־ח־ש", HebrewBinyan::Paal, Gender::Feminine, Number::Singular,
                      TenseAspect::Past));
        addEntry(verb("הגיע", "הגיע", "נ־ג־ע", HebrewBinyan::Hifil, Gender::Masculine, Number::Singular,
                      TenseAspect::Past));

        // Small bootstrap lexicon: grammar-independent nouns/adjectives for examples.
        for (auto entry: {
                 noun("נסיכה", "נסיכה", Gender::Feminine, Number::Singular, 20.0, true),
                 noun("נסיכות", "נסיכה", Gender::Feminine, Number::Plural, 20.0, true),
                 noun("מלך", "מלך", Gender::Masculine, Number::Singular, 20.0, true),
                 noun("כרוז", "כרוז", Gender::Masculine, Number::Singular, 20.0, true),
                 noun("נעל", "נעל", Gender::Feminine, Number::Singular),
                 noun("זכוכית", "זכוכית", Gender::Feminine, Number::Singular),
                 noun("רגל", "רגל", Gender::Feminine, Number::Singular),
                 noun("מידה", "מידה", Gender::Feminine, Number::Singular),
                 noun("מידת", "מידה", Gender::Feminine, Number::Singular),
                 noun("גובה", "גובה", Gender::Masculine, Number::Singular),
                 noun("אורך", "אורך", Gender::Masculine, Number::Singular),
                 noun("רוחב", "רוחב", Gender::Masculine, Number::Singular),
                 noun("משקל", "משקל", Gender::Masculine, Number::Singular),
                 noun("צבע", "צבע", Gender::Masculine, Number::Singular),
                 noun("שער", "שער", Gender::Masculine, Number::Singular),
                 noun("מגדל", "מגדל", Gender::Masculine, Number::Singular),
                 noun("ארמון", "ארמון", Gender::Masculine, Number::Singular),
                 noun("שליח", "שליח", Gender::Masculine, Number::Singular, 20.0, true),
                 noun("עורב", "עורב", Gender::Masculine, Number::Singular, 20.0, true),
                 noun("שועל", "שועל", Gender::Masculine, Number::Singular, 20.0, true),
                 noun("חרב", "חרב", Gender::Feminine, Number::Singular),
                 noun("קוסמת", "קוסמת", Gender::Feminine, Number::Singular, 20.0, true),
                 noun("שומרת", "שומר", Gender::Feminine, Number::Singular, 20.0, true),
                 noun("סף", "סף", Gender::Masculine, Number::Singular),
                 noun("כנף", "כנף", Gender::Feminine, Number::Singular),
                 noun("לב", "לב", Gender::Masculine, Number::Singular),
                 noun("שמלה", "שמלה", Gender::Feminine, Number::Singular),
             }) {
            addEntry(std::move(entry));
        }

        for (auto entry: {
                 adjective("גאה", "גאה", Gender::Feminine, Number::Singular),
                 adjective("מפונקת", "מפונק", Gender::Feminine, Number::Singular),
                 adjective("שקטה", "שקט", Gender::Feminine, Number::Singular),
                 adjective("נאמן", "נאמן", Gender::Masculine, Number::Singular),
                 adjective("עתיקה", "עתיק", Gender::Feminine, Number::Singular),
                 adjective("ערמומי", "ערמומי", Gender::Masculine, Number::Singular),
             }) {
            addEntry(std::move(entry));
        }
    }

    void Analyzer::addEntry(LexiconEntry entry) {
        const auto key = stripHebrewMarks(normalizeMaqaf(entry.surface));
        lexicon_.emplace(key, std::move(entry));
    }

    MorphLattice Analyzer::analyze(const std::vector<Lexeme>& lexemes) const {
        MorphLattice lattice;
        for (const auto& lexeme: lexemes) {
            if (lexeme.kind == Lexeme::Kind::Comma) {
                continue;
            }
            lattice.tokens.push_back(analyzeLexeme(lexeme));
        }
        return lattice;
    }

    SurfaceTokenAnalysis Analyzer::analyzeLexeme(const Lexeme& lexeme) const {
        SurfaceTokenAnalysis result;
        result.token = makeOrthographicToken(lexeme);
        result.candidates = analyzeToken(result.token);
        return result;
    }

    std::vector<MorphAnalysis> Analyzer::analyzeToken(const OrthographicToken& token) const {
        std::vector<MorphAnalysis> candidates;

        if (token.sourceKind == Lexeme::Kind::Number) {
            MorphAnalysis analysis;
            analysis.surface = token.surface;
            analysis.consonantal = token.consonantal;
            analysis.lemma = token.surface;
            analysis.pos = PartOfSpeech::Number;
            analysis.score = 100.0;
            analysis.provenance = AnalysisProvenance::ParserForced;
            candidates.push_back(std::move(analysis));
            return candidates;
        }

        if (token.sourceKind == Lexeme::Kind::String) {
            MorphAnalysis analysis;
            analysis.surface = token.surface;
            analysis.consonantal = token.consonantal;
            analysis.lemma = token.surface;
            analysis.pos = PartOfSpeech::StringLiteral;
            analysis.score = 100.0;
            analysis.provenance = AnalysisProvenance::ParserForced;
            candidates.push_back(std::move(analysis));
            return candidates;
        }

        auto append = [&](std::vector<MorphAnalysis> more) {
            for (auto& candidate: more) {
                candidates.push_back(std::move(candidate));
            }
        };

        append(lexiconCandidates(token));
        append(prefixCandidates(token));
        append(possessiveCandidates(token));
        append(maqafCandidates(token));
        append(oovCandidates(token));

        candidates = applyNiqqudPolicy(token, std::move(candidates));
        std::sort(candidates.begin(), candidates.end(), [](const auto& left, const auto& right) {
            return left.score > right.score;
        });
        return candidates;
    }

    std::vector<MorphAnalysis> Analyzer::lexiconCandidates(const OrthographicToken& token) const {
        std::vector<MorphAnalysis> candidates;
        const auto [begin, end] = lexicon_.equal_range(token.consonantal);
        for (auto it = begin; it != end; ++it) {
            candidates.push_back(fromEntry(token, it->second));
        }
        return candidates;
    }

    std::vector<MorphAnalysis> Analyzer::prefixCandidates(const OrthographicToken& token) const {
        std::vector<MorphAnalysis> candidates;
        const auto cps = decodeUtf8(token.consonantal);
        if (cps.size() < 2) {
            return candidates;
        }

        std::vector<PrefixAnalysis> prefixes;
        std::size_t index = 0;
        for (; index < cps.size() && index < 3; ++index) {
            auto prefix = prefixFor(cps[index]);
            if (!prefix) {
                break;
            }
            prefixes.push_back(std::move(*prefix));
            const std::vector<char32_t> restCps(cps.begin() + static_cast<long>(index + 1), cps.end());
            const auto rest = encodeUtf8(restCps);
            const auto [begin, end] = lexicon_.equal_range(rest);
            bool foundRest = false;
            for (auto it = begin; it != end; ++it) {
                foundRest = true;
                MorphAnalysis analysis = fromEntry(token, it->second);
                analysis.provenance = AnalysisProvenance::RuleDerived;
                analysis.prefixes = prefixes;
                analysis.score -= 2.0 + static_cast<double>(index);
                analysis.notes.push_back("prefix segmentation candidate");
                for (auto pit = prefixes.rbegin(); pit != prefixes.rend(); ++pit) {
                    analysis.morphemes.insert(analysis.morphemes.begin(), prefixMorpheme(*pit));
                }
                if (std::any_of(prefixes.begin(), prefixes.end(), [](const auto& p) {
                        return p.kind == PrefixKind::DefiniteArticle;
                    })) {
                    analysis.features.definiteness = Definiteness::Definite;
                }
                candidates.push_back(std::move(analysis));
            }

            if (!foundRest) {
                MorphAnalysis guess;
                guess.surface = token.surface;
                guess.consonantal = token.consonantal;
                guess.lemma = rest;
                guess.pos = PartOfSpeech::Noun;
                guess.features = features(Gender::Unknown, Number::Singular);
                guess.features.isRoleCandidate = true;
                guess.features.isObjectCandidate = true;
                guess.features.isPropertyCandidate = true;
                guess.provenance = AnalysisProvenance::OovGuess;
                guess.prefixes = prefixes;
                guess.score = 6.0 - static_cast<double>(index);
                guess.notes.push_back("prefix segmentation with OOV noun candidate");
                for (auto pit = prefixes.rbegin(); pit != prefixes.rend(); ++pit) {
                    guess.morphemes.insert(guess.morphemes.begin(), prefixMorpheme(*pit));
                }
                Morpheme stem;
                stem.surface = rest;
                stem.lemma = rest;
                stem.role = MorphemeRole::Noun;
                guess.morphemes.push_back(std::move(stem));
                candidates.push_back(std::move(guess));
            }
        }
        return candidates;
    }

    std::vector<MorphAnalysis> Analyzer::possessiveCandidates(const OrthographicToken& token) const {
        std::vector<MorphAnalysis> candidates;
        static const std::unordered_map<std::string, std::string> known = {
            {"רגלה", "רגל"}, {"רגלו", "רגל"},   {"לבה", "לב"},   {"לבו", "לב"},   {"שמה", "שם"},
            {"שמו", "שם"},   {"שמלתה", "שמלה"}, {"כנפו", "כנף"}, {"כנפה", "כנף"}, {"מידתה", "מידה"},
        };

        const auto it = known.find(token.consonantal);
        if (it == known.end()) {
            return candidates;
        }

        MorphAnalysis analysis;
        analysis.surface = token.surface;
        analysis.consonantal = token.consonantal;
        analysis.lemma = it->second;
        analysis.pos = PartOfSpeech::Noun;
        analysis.features = features(Gender::Unknown, Number::Singular);
        analysis.features.isPropertyCandidate = true;
        analysis.provenance = AnalysisProvenance::RuleDerived;
        analysis.score = 28.0;
        analysis.possessiveSuffix =
            PossessiveSuffix{token.consonantal.ends_with("ו") ? "ו" : "ה", Person::Third,
                             token.consonantal.ends_with("ו") ? Gender::Masculine : Gender::Feminine, Number::Singular};
        analysis.notes.push_back("possessive suffix candidate");
        Morpheme stem;
        stem.surface = token.surface;
        stem.lemma = it->second;
        stem.role = MorphemeRole::Noun;
        analysis.morphemes.push_back(std::move(stem));
        Morpheme suffix;
        suffix.surface = analysis.possessiveSuffix->surface;
        suffix.lemma = analysis.possessiveSuffix->surface;
        suffix.role = MorphemeRole::PossessiveSuffix;
        analysis.morphemes.push_back(std::move(suffix));
        candidates.push_back(std::move(analysis));
        return candidates;
    }

    std::vector<MorphAnalysis> Analyzer::maqafCandidates(const OrthographicToken& token) const {
        std::vector<MorphAnalysis> candidates;
        if (!token.hasMaqaf) {
            return candidates;
        }
        std::vector<std::string> parts;
        std::string current;
        for (const auto cp: decodeUtf8(token.consonantal)) {
            if (isMaqaf(cp)) {
                if (!current.empty()) {
                    parts.push_back(current);
                    current.clear();
                }
            } else {
                current += encodeUtf8(cp);
            }
        }
        if (!current.empty()) {
            parts.push_back(current);
        }
        if (parts.size() < 2) {
            return candidates;
        }

        MorphAnalysis analysis;
        analysis.surface = token.surface;
        analysis.consonantal = token.consonantal;
        analysis.lemma = joinSurface(parts, "־");
        analysis.pos = PartOfSpeech::Noun;
        analysis.features.state = State::Construct;
        analysis.features.definiteness = Definiteness::Construct;
        analysis.features.isObjectCandidate = true;
        analysis.provenance = AnalysisProvenance::RuleDerived;
        analysis.score = 26.0;
        analysis.notes.push_back("maqaf construct/compound candidate");
        for (const auto& part: parts) {
            Morpheme morpheme;
            morpheme.surface = part;
            morpheme.lemma = part.starts_with("ה") ? part.substr(std::string("ה").size()) : part;
            morpheme.role = MorphemeRole::Noun;
            analysis.morphemes.push_back(std::move(morpheme));
        }
        candidates.push_back(std::move(analysis));
        return candidates;
    }

    std::vector<MorphAnalysis> Analyzer::oovCandidates(const OrthographicToken& token) const {
        std::vector<MorphAnalysis> candidates;
        if (token.sourceKind == Lexeme::Kind::AsciiWord) {
            MorphAnalysis ascii;
            ascii.surface = token.surface;
            ascii.consonantal = token.surface;
            ascii.lemma = token.surface;
            ascii.pos = PartOfSpeech::Name;
            ascii.features.isNameCandidate = true;
            ascii.provenance = AnalysisProvenance::OovGuess;
            ascii.score = 12.0;
            ascii.notes.push_back("ASCII external/name candidate");
            candidates.push_back(std::move(ascii));
            return candidates;
        }

        MorphAnalysis name;
        name.surface = token.surface;
        name.consonantal = token.consonantal;
        name.lemma = token.consonantal;
        name.pos = PartOfSpeech::Name;
        name.features.isNameCandidate = true;
        name.provenance = AnalysisProvenance::OovGuess;
        name.score = 9.0;
        name.notes.push_back("unknown Hebrew word can be a story name");
        candidates.push_back(name);

        MorphAnalysis nounGuess = name;
        nounGuess.pos = PartOfSpeech::Noun;
        nounGuess.features.isRoleCandidate = true;
        nounGuess.features.isObjectCandidate = true;
        nounGuess.features.isPropertyCandidate = true;
        nounGuess.score = 7.0;
        nounGuess.notes = {"unknown Hebrew word can be a noun/role"};
        if (token.consonantal.ends_with("ות")) {
            nounGuess.features.gender = Gender::Feminine;
            nounGuess.features.number = Number::Plural;
        } else if (token.consonantal.ends_with("ים")) {
            nounGuess.features.gender = Gender::Masculine;
            nounGuess.features.number = Number::Plural;
        } else if (token.consonantal.ends_with("ה") || token.consonantal.ends_with("ת")) {
            nounGuess.features.gender = Gender::Feminine;
            nounGuess.features.number = Number::Singular;
        } else {
            nounGuess.features.gender = Gender::Unknown;
            nounGuess.features.number = Number::Singular;
        }
        candidates.push_back(std::move(nounGuess));
        return candidates;
    }

    std::vector<MorphAnalysis> Analyzer::applyNiqqudPolicy(const OrthographicToken& token,
                                                           std::vector<MorphAnalysis> candidates) const {
        if (!token.hasNiqqud) {
            return candidates;
        }

        bool anyLexicalMatch = false;
        for (auto& candidate: candidates) {
            if (candidate.consonantal == token.consonantal) {
                candidate.score += 20.0;
                candidate.provenance = AnalysisProvenance::NiqqudFiltered;
                candidate.notes.push_back("niqqud/consonantal form matched this candidate");
                anyLexicalMatch = true;
            }
        }
        if (!anyLexicalMatch) {
            for (auto& candidate: candidates) {
                candidate.notes.push_back("HN002: niqqud present but no known candidate matched it precisely");
            }
        }
        return candidates;
    }

    const MorphAnalysis* bestCandidate(const SurfaceTokenAnalysis& token) {
        if (token.candidates.empty()) {
            return nullptr;
        }
        return &token.candidates.front();
    }

    const MorphAnalysis* bestCandidateWithPos(const SurfaceTokenAnalysis& token, PartOfSpeech pos) {
        for (const auto& candidate: token.candidates) {
            if (candidate.pos == pos) {
                return &candidate;
            }
        }
        return bestCandidate(token);
    }

    void dumpLattice(std::ostream& out, const MorphLattice& lattice) {
        for (const auto& token: lattice.tokens) {
            out << token.token.line << ':' << token.token.column << " " << token.token.surface
                << " consonantal=" << token.token.consonantal << " niqqud=" << (token.token.hasNiqqud ? "yes" : "no")
                << '\n';
            for (const auto& candidate: token.candidates) {
                out << "  - pos=" << toString(candidate.pos) << " lemma=" << candidate.lemma << " score=" << std::fixed
                    << std::setprecision(1) << candidate.score << " provenance=" << toString(candidate.provenance)
                    << " gender=" << toString(candidate.features.gender)
                    << " number=" << toString(candidate.features.number)
                    << " binyan=" << toString(candidate.features.binyan);
                if (candidate.root) {
                    out << " root=" << *candidate.root;
                }
                if (candidate.possessiveSuffix) {
                    out << " possessive=" << toString(candidate.possessiveSuffix->person) << '/'
                        << toString(candidate.possessiveSuffix->gender) << '/'
                        << toString(candidate.possessiveSuffix->number);
                }
                out << '\n';
                for (const auto& note: candidate.notes) {
                    out << "      note: " << note << '\n';
                }
            }
        }
    }

} // namespace hyh::hebrew
