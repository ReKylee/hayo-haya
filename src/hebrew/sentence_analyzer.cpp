#include "hebrew/sentence_ir.hpp"

#include "hebrew/selection.hpp"

#include <algorithm>
#include <cstdlib>
#include <ostream>

namespace hyh::hebrew {
    namespace {

        std::string rawText(const RawLine& line) {
            std::string text;
            for (const auto& lexeme: line.lexemes) {
                if (!text.empty() && lexeme.kind != Lexeme::Kind::Comma) {
                    text += ' ';
                }
                text += lexeme.text;
            }
            return text;
        }

        HebrewSentence baseSentence(const MorphLattice& lattice, const RawLine& line) {
            HebrewSentence sentence;
            sentence.line = line.line;
            sentence.rawText = rawText(line);
            sentence.tokens = lattice.tokens;
            return sentence;
        }

        const SurfaceTokenAnalysis* tokenAt(const MorphLattice& lattice, std::size_t index) {
            if (index >= lattice.tokens.size()) {
                return nullptr;
            }
            return &lattice.tokens[index];
        }

        const MorphAnalysis* selected(const MorphLattice& lattice, std::size_t index,
                                      ExpectedCategory category = ExpectedCategory::Any) {
            const auto* token = tokenAt(lattice, index);
            if (!token) {
                return nullptr;
            }
            SelectionEngine engine;
            GrammarExpectation expectation;
            expectation.category = category;
            return engine.select(*token, expectation).analysis;
        }

        bool lemmaIs(const MorphLattice& lattice, std::size_t index, const std::string& lemma) {
            const auto* analysis = selected(lattice, index);
            return analysis && analysis->lemma == lemma;
        }

        bool surfaceIs(const MorphLattice& lattice, std::size_t index, const std::string& surface) {
            const auto* token = tokenAt(lattice, index);
            return token && token->token.consonantal == surface;
        }

        bool hasPrefixKind(const MorphAnalysis& analysis, PrefixKind kind) {
            return std::any_of(analysis.prefixes.begin(), analysis.prefixes.end(), [&](const PrefixAnalysis& prefix) {
                return prefix.kind == kind;
            });
        }

        bool isCopula(const MorphAnalysis* analysis) {
            return analysis && analysis->pos == PartOfSpeech::Verb && analysis->lemma == "היה";
        }

        bool isSpeechVerb(const MorphAnalysis* analysis) {
            return analysis && analysis->pos == PartOfSpeech::Verb
                && (analysis->lemma == "קרא" || analysis->lemma == "אמר" || analysis->lemma == "לחש");
        }

        bool isImportAliasMarker(const MorphLattice& lattice, std::size_t index) {
            const auto* token = tokenAt(lattice, index);
            if (!token) {
                return false;
            }
            return token->token.consonantal == "שמו" || token->token.consonantal == "ושמו"
                || token->token.consonantal == "שמה" || token->token.consonantal == "ושמה";
        }

        std::size_t findCopula(const MorphLattice& lattice, std::size_t begin) {
            for (std::size_t i = begin; i < lattice.tokens.size(); ++i) {
                if (isCopula(selected(lattice, i, ExpectedCategory::Verb))) {
                    return i;
                }
            }
            return lattice.tokens.size();
        }

        std::string stripDefiniteArticle(std::string value) {
            if (value.starts_with("ה") && value.size() > std::string("ה").size()) {
                return value.substr(std::string("ה").size());
            }
            return value;
        }

        bool looksDefinite(const NounPhrase& phrase) {
            return phrase.definiteness == Definiteness::Definite || phrase.surface.starts_with("ה");
        }

        NounPhrase nounPhraseFromToken(const SurfaceTokenAnalysis& token) {
            NounPhrase phrase;
            phrase.surface = token.token.surface;
            if (const auto* analysis = bestCandidateWithPos(token, PartOfSpeech::Noun)) {
                phrase.headLemma = analysis->lemma;
                phrase.gender = analysis->features.gender;
                phrase.number = analysis->features.number;
                phrase.definiteness = analysis->features.definiteness;
                phrase.isConstruct = analysis->features.state == State::Construct || token.token.hasMaqaf;
                phrase.isAnimateCandidate = analysis->features.isAnimateCandidate;
                phrase.possessive = analysis->possessiveSuffix;
                for (const auto& morpheme: analysis->morphemes) {
                    if (!morpheme.lemma.empty()) {
                        phrase.parts.push_back(morpheme.lemma);
                    }
                }
            }
            if (phrase.headLemma.empty()) {
                phrase.headLemma = token.token.consonantal;
            }
            if (phrase.parts.empty()) {
                phrase.parts.push_back(phrase.headLemma);
            }
            return phrase;
        }

        VerbReading verbFromToken(const SurfaceTokenAnalysis& token) {
            VerbReading reading;
            reading.surface = token.token.surface;
            if (const auto* analysis = bestCandidateWithPos(token, PartOfSpeech::Verb)) {
                reading.lemma = analysis->lemma;
                reading.root = analysis->root;
                reading.binyan = analysis->features.binyan;
                reading.gender = analysis->features.gender;
                reading.number = analysis->features.number;
                reading.person = analysis->features.person;
                reading.tenseAspect = analysis->features.tenseAspect;
            }
            return reading;
        }

        std::string entityNameFromPhrase(const NounPhrase& phrase) {
            if (!phrase.headLemma.empty()) {
                return stripDefiniteArticle(phrase.headLemma);
            }
            return stripDefiniteArticle(phrase.surface);
        }

        std::string joinProperty(const std::vector<std::string>& parts) {
            std::string out;
            for (const auto& part: parts) {
                if (part.empty()) {
                    continue;
                }
                if (!out.empty()) {
                    out += "־";
                }
                out += part;
            }
            return out;
        }

        [[maybe_unused]] std::string tokenLemma(const SurfaceTokenAnalysis& token,
                                                ExpectedCategory category = ExpectedCategory::Noun) {
            SelectionEngine engine;
            GrammarExpectation expectation;
            expectation.category = category;
            const auto result = engine.select(token, expectation);
            if (result.analysis && !result.analysis->lemma.empty()) {
                return result.analysis->lemma;
            }
            return token.token.consonantal;
        }

        bool isNumericLiteral(const MorphLattice& lattice, std::size_t index) {
            const auto* analysis = selected(lattice, index, ExpectedCategory::Number);
            return analysis && analysis->pos == PartOfSpeech::Number;
        }

        bool isOneNumber(const MorphAnalysis& analysis) {
            return analysis.lemma == "1" || analysis.surface == "1" || analysis.surface == "אחד"
                || analysis.surface == "אחת";
        }

        const MorphAnalysis* candidateWithExactPos(const SurfaceTokenAnalysis& token, PartOfSpeech pos) {
            for (const auto& candidate: token.candidates) {
                if (candidate.pos == pos) {
                    return &candidate;
                }
            }
            return nullptr;
        }

        bool canActAsQuantityNoun(const SurfaceTokenAnalysis& token) {
            const auto* best = bestCandidate(token);
            return best && (best->pos == PartOfSpeech::Noun || best->pos == PartOfSpeech::Name);
        }

        void addQuantityPhraseDiagnostics(HebrewSentence& sentence, const MorphLattice& lattice) {
            for (std::size_t i = 0; i + 1 < lattice.tokens.size(); ++i) {
                const auto* firstToken = tokenAt(lattice, i);
                const auto* secondToken = tokenAt(lattice, i + 1);
                if (!firstToken || !secondToken) {
                    continue;
                }

                const auto* firstNumber = candidateWithExactPos(*firstToken, PartOfSpeech::Number);
                const auto* firstNoun = candidateWithExactPos(*firstToken, PartOfSpeech::Noun);
                const auto* secondNumber = candidateWithExactPos(*secondToken, PartOfSpeech::Number);
                const auto* secondNoun = candidateWithExactPos(*secondToken, PartOfSpeech::Noun);

                if (firstNoun && !canActAsQuantityNoun(*firstToken)) {
                    firstNoun = nullptr;
                }
                if (secondNoun && !canActAsQuantityNoun(*secondToken)) {
                    secondNoun = nullptr;
                }

                if (firstNoun && secondNumber) {
                    if (!isOneNumber(*secondNumber)) {
                        sentence.diagnostics.push_back("HN005 quantity phrase order mismatch: noun-before-number is "
                                                       "only used for one, got '"
                                                       + lattice.tokens[i].token.surface + " "
                                                       + lattice.tokens[i + 1].token.surface + "'");
                    }
                    if (firstNoun->features.number != Number::Unknown
                        && firstNoun->features.number != Number::Singular) {
                        sentence.diagnostics
                            .push_back("HN005 quantity phrase number mismatch: one expects a singular noun, got '"
                                       + lattice.tokens[i].token.surface + " " + lattice.tokens[i + 1].token.surface
                                       + "'");
                    }
                }

                if (firstNumber && secondNoun) {
                    if (isOneNumber(*firstNumber)) {
                        sentence.diagnostics
                            .push_back("HN005 quantity phrase order mismatch: one follows the singular noun, got '"
                                       + lattice.tokens[i].token.surface + " " + lattice.tokens[i + 1].token.surface
                                       + "'");
                    } else if (secondNoun->features.number != Number::Unknown
                               && secondNoun->features.number != Number::Plural) {
                        sentence.diagnostics.push_back("HN005 quantity phrase number mismatch: numbers above one "
                                                       "expect a plural noun, got '"
                                                       + lattice.tokens[i].token.surface + " "
                                                       + lattice.tokens[i + 1].token.surface + "'");
                    }
                }
            }
        }

        bool isStringLiteralToken(const MorphLattice& lattice, std::size_t index) {
            const auto* token = tokenAt(lattice, index);
            return token && token->token.sourceKind == Lexeme::Kind::String;
        }

        int integerAt(const MorphLattice& lattice, std::size_t index) {
            const auto* token = tokenAt(lattice, index);
            if (!token) {
                return 0;
            }
            return std::atoi(token->token.consonantal.c_str());
        }

        std::size_t skipComparisonPrefix(const MorphLattice& lattice, std::size_t begin, std::size_t end,
                                         bool& comparisonPrefixed) {
            if (begin >= end) {
                return begin;
            }
            const auto* analysis = selected(lattice, begin);
            if (analysis && hasPrefixKind(*analysis, PrefixKind::As)) {
                comparisonPrefixed = true;
                return begin;
            }
            if (lemmaIs(lattice, begin, "כ") || surfaceIs(lattice, begin, "כ")) {
                comparisonPrefixed = true;
                return begin + 1;
            }
            return begin;
        }

        NumberExpressionIr numberExpressionFromRange(const MorphLattice& lattice, std::size_t begin, std::size_t end) {
            NumberExpressionIr expr;
            if (begin >= end || begin >= lattice.tokens.size()) {
                return expr;
            }

            begin = skipComparisonPrefix(lattice, begin, end, expr.comparisonPrefixed);
            if (begin >= end || begin >= lattice.tokens.size()) {
                return expr;
            }

            expr.rawSurface.clear();
            for (std::size_t i = begin; i < end && i < lattice.tokens.size(); ++i) {
                if (!expr.rawSurface.empty()) {
                    expr.rawSurface += ' ';
                }
                expr.rawSurface += lattice.tokens[i].token.surface;
            }

            if (end == begin + 1 && isNumericLiteral(lattice, begin)) {
                expr.kind = NumberExpressionIr::Kind::Literal;
                expr.literal = integerAt(lattice, begin);
                return expr;
            }

            const auto* firstToken = tokenAt(lattice, begin);
            if (!firstToken) {
                return expr;
            }
            const auto firstPhrase = nounPhraseFromToken(*firstToken);

            // Possessive noun/property by itself, e.g. "כנפו", "רגלה".
            if (end == begin + 1 && firstPhrase.possessive) {
                expr.kind = NumberExpressionIr::Kind::CurrentActorProperty;
                expr.propertyName = firstPhrase.headLemma;
                expr.phraseLemmas.push_back(firstPhrase.headLemma);
                return expr;
            }

            // Generic construct/property form:
            //   <property-head> <possessive-noun>     -> current actor property "head־noun"
            //   <property-head> <indefinite noun>     -> current actor property "head־noun"
            //   <property-head> <definite entity ref> -> entity property "head" on object/entity
            //
            // The indefinite form names a property kind, not an object. For example,
            // "מידת נעל" is one property phrase; "מידת הנעל" can refer to a concrete
            // shoe entity already present in discourse.
            if (end >= begin + 2) {
                const auto* secondToken = tokenAt(lattice, begin + 1);
                if (!secondToken) {
                    return expr;
                }
                const auto secondPhrase = nounPhraseFromToken(*secondToken);
                expr.phraseLemmas.push_back(firstPhrase.headLemma);
                expr.phraseLemmas.push_back(secondPhrase.headLemma);

                if (secondPhrase.possessive) {
                    expr.kind = NumberExpressionIr::Kind::CurrentActorProperty;
                    expr.propertyName = joinProperty({firstPhrase.headLemma, secondPhrase.headLemma});
                    return expr;
                }

                if (!looksDefinite(secondPhrase)) {
                    expr.kind = NumberExpressionIr::Kind::CurrentActorProperty;
                    expr.propertyName = joinProperty({firstPhrase.headLemma, secondPhrase.headLemma});
                    return expr;
                }

                expr.kind = NumberExpressionIr::Kind::EntityProperty;
                expr.propertyName = firstPhrase.headLemma;
                expr.entityName = entityNameFromPhrase(secondPhrase);
                expr.entitySurface = secondPhrase.surface;
                expr.entityHeadLemma = secondPhrase.headLemma;
                expr.entityDefinite = looksDefinite(secondPhrase);
                expr.entityGender = secondPhrase.gender;
                expr.entityNumber = secondPhrase.number;
                return expr;
            }

            // Fallback: a bare noun in expression position is a current actor property.
            expr.kind = NumberExpressionIr::Kind::CurrentActorProperty;
            expr.propertyName = firstPhrase.headLemma;
            expr.phraseLemmas.push_back(firstPhrase.headLemma);
            return expr;
        }

        std::string ownerDebugName(const NumberExpressionIr& expr) {
            switch (expr.kind) {
            case NumberExpressionIr::Kind::CurrentActorProperty:
                return "<current-actor>";
            case NumberExpressionIr::Kind::EntityProperty:
                return expr.entityName.empty() ? "<unknown-entity>" : expr.entityName;
            case NumberExpressionIr::Kind::Literal:
                return "<literal>";
            case NumberExpressionIr::Kind::Unknown:
                return "<unknown>";
            }
            return "<unknown>";
        }

        std::string sentenceKindName(HebrewSentenceKind kind) {
            switch (kind) {
            case HebrewSentenceKind::RoleIntroduction:
                return "RoleIntroduction";
            case HebrewSentenceKind::PropertyAssignment:
                return "PropertyAssignment";
            case HebrewSentenceKind::ForEachRole:
                return "ForEachRole";
            case HebrewSentenceKind::Condition:
                return "Condition";
            case HebrewSentenceKind::Speech:
                return "Speech";
            case HebrewSentenceKind::Import:
                return "Import";
            case HebrewSentenceKind::TeachingDirective:
                return "TeachingDirective";
            case HebrewSentenceKind::NarrativeEvent:
                return "NarrativeEvent";
            case HebrewSentenceKind::Unknown:
                return "Unknown";
            }
            return "Unknown";
        }

    } // namespace

    HebrewSentence SentenceAnalyzer::analyze(const MorphLattice& lattice, const RawLine& line) const {
        // Order matters. Condition and speech markers are closed-class syntax and must
        // win before the generic copular role/property templates. Otherwise a line
        // like "כאשר כנפו היה כגובה השער" looks like "כאשר היה גובה" and is
        // incorrectly lowered as a role introduction.
        for (const auto& attempt: {
                 tryCondition(lattice, line),
                 tryImport(lattice, line),
                 trySpeech(lattice, line),
                 tryForEachRole(lattice, line),
                 tryPropertyAssignment(lattice, line),
                 tryRoleIntroduction(lattice, line),
             }) {
            if (attempt.kind != HebrewSentenceKind::Unknown) {
                auto sentence = attempt;
                addQuantityPhraseDiagnostics(sentence, lattice);
                return sentence;
            }
        }
        auto sentence = narrativeEvent(lattice, line);
        addQuantityPhraseDiagnostics(sentence, lattice);
        return sentence;
    }

    HebrewSentence SentenceAnalyzer::tryImport(const MorphLattice& lattice, const RawLine& line) const {
        auto sentence = baseSentence(lattice, line);
        if (lattice.tokens.size() < 5 || !lemmaIs(lattice, 0, "מן")) {
            return sentence;
        }

        std::size_t importedIndex = lattice.tokens.size();
        for (std::size_t i = 1; i < lattice.tokens.size(); ++i) {
            if (lattice.tokens[i].token.sourceKind == Lexeme::Kind::AsciiWord) {
                importedIndex = i;
                break;
            }
        }
        if (importedIndex == lattice.tokens.size() || importedIndex + 2 >= lattice.tokens.size()) {
            return sentence;
        }

        bool hasArrivalVerb = false;
        for (std::size_t i = 1; i < importedIndex; ++i) {
            const auto* verb = selected(lattice, i, ExpectedCategory::Verb);
            if (verb && verb->lemma == "הגיע") {
                hasArrivalVerb = true;
                break;
            }
        }
        if (!hasArrivalVerb || !isImportAliasMarker(lattice, importedIndex + 1)) {
            return sentence;
        }

        const auto* aliasToken = tokenAt(lattice, importedIndex + 2);
        if (!aliasToken || aliasToken->token.sourceKind != Lexeme::Kind::HebrewWord) {
            return sentence;
        }

        sentence.kind = HebrewSentenceKind::Import;
        sentence.importedName = lattice.tokens[importedIndex].token.consonantal;
        sentence.importAlias = aliasToken->token.consonantal;
        return sentence;
    }

    HebrewSentence SentenceAnalyzer::tryRoleIntroduction(const MorphLattice& lattice, const RawLine& line) const {
        auto sentence = baseSentence(lattice, line);
        if (lattice.tokens.size() < 3) {
            return sentence;
        }

        const auto copulaIndex = findCopula(lattice, 1);
        if (copulaIndex == lattice.tokens.size() || copulaIndex + 1 >= lattice.tokens.size()) {
            return sentence;
        }

        const auto* name = selected(lattice, 0, ExpectedCategory::Name);
        const auto* role = selected(lattice, copulaIndex + 1, ExpectedCategory::RoleNoun);
        if (!name || !role || role->pos != PartOfSpeech::Noun) {
            return sentence;
        }

        sentence.kind = HebrewSentenceKind::RoleIntroduction;
        sentence.subjectName = lattice.tokens[0].token.consonantal;
        sentence.role = nounPhraseFromToken(lattice.tokens[copulaIndex + 1]);
        for (std::size_t i = copulaIndex + 2; i < lattice.tokens.size(); ++i) {
            if (const auto* attribute = selected(lattice, i, ExpectedCategory::Any);
                attribute && attribute->pos == PartOfSpeech::Adjective) {
                sentence.attributes.push_back(attribute->lemma);
            }
        }
        return sentence;
    }

    HebrewSentence SentenceAnalyzer::tryPropertyAssignment(const MorphLattice& lattice, const RawLine& line) const {
        auto sentence = baseSentence(lattice, line);
        if (lattice.tokens.size() < 3) {
            return sentence;
        }

        const auto copulaIndex = findCopula(lattice, 1);
        if (copulaIndex == lattice.tokens.size() || copulaIndex == 0 || copulaIndex + 1 >= lattice.tokens.size()) {
            return sentence;
        }

        auto target = numberExpressionFromRange(lattice, 0, copulaIndex);
        if (target.kind == NumberExpressionIr::Kind::Unknown || target.kind == NumberExpressionIr::Kind::Literal) {
            return sentence;
        }

        auto value = numberExpressionFromRange(lattice, copulaIndex + 1, lattice.tokens.size());
        if (value.kind != NumberExpressionIr::Kind::Literal) {
            return sentence;
        }

        sentence.kind = HebrewSentenceKind::PropertyAssignment;
        sentence.leftNumber = std::move(target);
        sentence.literalValue = value.literal;
        return sentence;
    }

    HebrewSentence SentenceAnalyzer::tryForEachRole(const MorphLattice& lattice, const RawLine& line) const {
        auto sentence = baseSentence(lattice, line);
        if (lattice.tokens.size() < 4 || (!surfaceIs(lattice, 0, "בזו") && !surfaceIs(lattice, 0, "בזה"))) {
            return sentence;
        }

        std::size_t everyIndex = lattice.tokens.size();
        for (std::size_t i = 0; i < lattice.tokens.size(); ++i) {
            if (lemmaIs(lattice, i, "כל")) {
                everyIndex = i;
                break;
            }
        }
        if (everyIndex == lattice.tokens.size() || everyIndex == 0 || everyIndex + 1 >= lattice.tokens.size()) {
            return sentence;
        }

        auto* roleToken = tokenAt(lattice, everyIndex + 1);
        const auto* role = selected(lattice, everyIndex + 1, ExpectedCategory::RoleNoun);
        const auto* verb = selected(lattice, everyIndex - 1, ExpectedCategory::Verb);
        if (!roleToken || !role || !verb || role->pos != PartOfSpeech::Noun || verb->pos != PartOfSpeech::Verb) {
            return sentence;
        }

        sentence.kind = HebrewSentenceKind::ForEachRole;
        sentence.role = nounPhraseFromToken(*roleToken);
        sentence.predicate = verbFromToken(*tokenAt(lattice, everyIndex - 1));

        if (verb->features.number != Number::Unknown && verb->features.number != Number::Singular) {
            sentence.diagnostics.push_back("HN004 number agreement mismatch: quantified singular 'כל "
                                           + sentence.role.headLemma + "' expects a singular verb, got "
                                           + verb->surface);
        }

        for (std::size_t i = everyIndex + 2; i < lattice.tokens.size(); ++i) {
            if (lemmaIs(lattice, i, "אל") && i + 1 < lattice.tokens.size()) {
                sentence.object = nounPhraseFromToken(lattice.tokens[i + 1]);
                break;
            }
        }
        return sentence;
    }

    HebrewSentence SentenceAnalyzer::tryCondition(const MorphLattice& lattice, const RawLine& line) const {
        auto sentence = baseSentence(lattice, line);
        if (lattice.tokens.size() < 4 || !lemmaIs(lattice, 0, "כאשר")) {
            return sentence;
        }

        const auto copulaIndex = findCopula(lattice, 1);
        if (copulaIndex == lattice.tokens.size() || copulaIndex == 1 || copulaIndex + 1 >= lattice.tokens.size()) {
            return sentence;
        }

        auto left = numberExpressionFromRange(lattice, 1, copulaIndex);
        auto right = numberExpressionFromRange(lattice, copulaIndex + 1, lattice.tokens.size());
        if (left.kind == NumberExpressionIr::Kind::Unknown || right.kind == NumberExpressionIr::Kind::Unknown) {
            return sentence;
        }

        sentence.kind = HebrewSentenceKind::Condition;
        sentence.leftNumber = std::move(left);
        sentence.rightNumber = std::move(right);
        return sentence;
    }

    HebrewSentence SentenceAnalyzer::trySpeech(const MorphLattice& lattice, const RawLine& line) const {
        auto sentence = baseSentence(lattice, line);
        if (lattice.tokens.size() < 3) {
            return sentence;
        }
        std::size_t verbIndex = lattice.tokens.size();
        std::size_t stringIndex = lattice.tokens.size();
        for (std::size_t i = 1; i < lattice.tokens.size(); ++i) {
            if (verbIndex == lattice.tokens.size()
                && isSpeechVerb(selected(lattice, i, ExpectedCategory::SpeechVerb))) {
                verbIndex = i;
            }
            // Do not use SelectionEngine here: it deliberately falls back to the best
            // candidate when no category matches, which made the speech verb itself look
            // like a string. For quoted text, the raw lexer kind is the reliable signal.
            if (isStringLiteralToken(lattice, i)) {
                stringIndex = i;
                break;
            }
        }
        if (verbIndex == lattice.tokens.size() || stringIndex == lattice.tokens.size()) {
            return sentence;
        }
        sentence.kind = HebrewSentenceKind::Speech;
        sentence.speakerName = lattice.tokens[0].token.consonantal;
        sentence.predicate = verbFromToken(lattice.tokens[verbIndex]);
        sentence.speechText = lattice.tokens[stringIndex].token.surface;
        return sentence;
    }

    HebrewSentence SentenceAnalyzer::narrativeEvent(const MorphLattice& lattice, const RawLine& line) const {
        auto sentence = baseSentence(lattice, line);
        sentence.kind = HebrewSentenceKind::NarrativeEvent;
        return sentence;
    }

    void dumpSentenceIr(std::ostream& out, const HebrewSentence& sentence) {
        out << sentence.line << ": " << sentenceKindName(sentence.kind) << " :: " << sentence.rawText << '\n';
        if (!sentence.subjectName.empty()) {
            out << "  subject/name: " << sentence.subjectName << '\n';
        }
        if (!sentence.role.headLemma.empty()) {
            out << "  role/head: " << sentence.role.headLemma << " gender=" << toString(sentence.role.gender)
                << " number=" << toString(sentence.role.number) << '\n';
        }
        if (!sentence.predicate.lemma.empty()) {
            out << "  predicate: " << sentence.predicate.lemma
                << " root=" << (sentence.predicate.root ? *sentence.predicate.root : "")
                << " binyan=" << toString(sentence.predicate.binyan)
                << " number=" << toString(sentence.predicate.number) << '\n';
        }
        if (!sentence.leftNumber.propertyName.empty()
            || sentence.leftNumber.kind == NumberExpressionIr::Kind::Literal) {
            out << "  left-number: kind=" << static_cast<int>(sentence.leftNumber.kind)
                << " property=" << sentence.leftNumber.propertyName << " owner=" << ownerDebugName(sentence.leftNumber)
                << " raw=" << sentence.leftNumber.rawSurface << '\n';
        }
        if (!sentence.rightNumber.propertyName.empty()
            || sentence.rightNumber.kind == NumberExpressionIr::Kind::Literal) {
            out << "  right-number: kind=" << static_cast<int>(sentence.rightNumber.kind)
                << " property=" << sentence.rightNumber.propertyName
                << " owner=" << ownerDebugName(sentence.rightNumber) << " raw=" << sentence.rightNumber.rawSurface
                << '\n';
        }
        if (!sentence.speechText.empty()) {
            out << "  speech: " << sentence.speechText << '\n';
        }
        if (!sentence.importedName.empty()) {
            out << "  import: " << sentence.importedName << " as " << sentence.importAlias << '\n';
        }
        for (const auto& diagnostic: sentence.diagnostics) {
            out << "  diagnostic: " << diagnostic << '\n';
        }
    }

} // namespace hyh::hebrew
