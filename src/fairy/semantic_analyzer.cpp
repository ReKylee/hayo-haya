#include "fairy/semantic_analyzer.hpp"

#include <algorithm>
#include <sstream>

#include "fairy/discourse_resolver.hpp"
#include "fairy/mirror_questions.hpp"
#include "fairy/mirror_type_resolver.hpp"

namespace hyh::fairy {
    namespace {

        bool contains(const std::vector<std::string>& values, const std::string& value) {
            return std::ranges::find(values, value) != values.end();
        }

        std::string stripDefiniteArticle(std::string value) {
            if (value.starts_with("ה") && value.size() > std::string("ה").size()) {
                return value.substr(std::string("ה").size());
            }
            return value;
        }

        bool isDefiniteReference(const std::string& value) {
            return value.starts_with("ה") && value.size() > std::string("ה").size();
        }

        std::string aliasKey(const std::string& value) {
            return stripDefiniteArticle(value);
        }

        std::string headLemmaFromName(const std::string& name) {
            const auto normalized = stripDefiniteArticle(name);
            const auto maqaf = normalized.find("־");
            if (maqaf == std::string::npos) {
                return normalized;
            }
            return stripDefiniteArticle(normalized.substr(0, maqaf));
        }

        std::string ambiguityMessage(const hyh::hebrew::NumberExpressionIr& ir, const ResolutionResult& result) {
            std::ostringstream out;
            out << "HN010 ambiguous reference: '" << (ir.entitySurface.empty() ? ir.entityName : ir.entitySurface)
                << "' could refer to ";
            for (std::size_t i = 0; i < result.candidates.size() && i < 3; ++i) {
                if (i > 0) {
                    out << ", ";
                }
                out << result.candidates[i].entityName << " (score " << result.candidates[i].score << ')';
            }
            return out.str();
        }

        bool canBeImplicitPossessiveOwner(const Program& program, const Statement& statement) {
            const auto entity = program.entities.find(statement.entityName);
            return entity != program.entities.end() && entity->second.canBeImplicitPossessiveOwner;
        }

        const ImportBinding* findImport(const Program& program, const std::string& aliasName) {
            const auto it = program.importsByAlias.find(aliasKey(aliasName));
            return it == program.importsByAlias.end() ? nullptr : &it->second;
        }

        bool tokenHasSpeechVerb(const hyh::hebrew::SurfaceTokenAnalysis& token) {
            return std::ranges::any_of(token.candidates, [](const auto& candidate) {
                return candidate.pos == hyh::hebrew::PartOfSpeech::Verb
                    && (candidate.lemma == "קרא" || candidate.lemma == "אמר" || candidate.lemma == "לחש");
            });
        }

        bool looksLikeBareImportedCall(const Program& program, const hyh::hebrew::HebrewSentence& sentence) {
            if (sentence.tokens.size() < 2 || isDefiniteReference(sentence.tokens.front().token.consonantal)
                || !findImport(program, sentence.tokens.front().token.consonantal)) {
                return false;
            }

            return std::ranges::any_of(sentence.tokens.begin() + 1, sentence.tokens.end(), tokenHasSpeechVerb);
        }

    } // namespace

    Program SemanticAnalyzer::analyze(const std::vector<SemanticInputNode>& nodes) const {
        Program program;
        lowerNodes(nodes, program, program.statements);
        MirrorTypeResolver{}.resolve(program);
        return program;
    }

    void SemanticAnalyzer::lowerNodes(const std::vector<SemanticInputNode>& nodes, Program& program,
                                      std::vector<Statement>& out, std::string currentEntityName) const {
        for (const auto& node: nodes) {
            Statement statement = lowerSentence(node, program, currentEntityName);
            if (!node.children.empty()) {
                lowerNodes(node.children, program, statement.body);
            }
            if (!statement.entityName.empty() && canBeImplicitPossessiveOwner(program, statement)) {
                currentEntityName = statement.entityName;
            }
            out.push_back(std::move(statement));
        }
    }

    void SemanticAnalyzer::registerEntity(Program& program, const std::string& entityName, std::size_t line,
                                          hyh::hebrew::Gender gender, hyh::hebrew::Number number) const {
        if (entityName.empty()) {
            return;
        }

        auto& entity = program.entities[entityName];
        entity.name = entityName;
        if (entity.headLemma.empty()) {
            entity.headLemma = headLemmaFromName(entityName);
        }
        if (entity.gender == hyh::hebrew::Gender::Unknown && gender != hyh::hebrew::Gender::Unknown) {
            entity.gender = gender;
        }
        if (entity.number == hyh::hebrew::Number::Unknown && number != hyh::hebrew::Number::Unknown) {
            entity.number = number;
        }
        if (entity.firstMentionLine == 0) {
            entity.firstMentionLine = line;
        }
        entity.lastMentionLine = line;
        ++entity.mentionCount;
    }

    void SemanticAnalyzer::registerRole(Program& program, const std::string& entityName, const std::string& roleName,
                                        const std::vector<std::string>& attributes, std::size_t line,
                                        hyh::hebrew::Gender gender, hyh::hebrew::Number number,
                                        bool canBeImplicitPossessiveOwner) const {
        registerEntity(program, entityName, line, gender, number);

        auto& entity = program.entities[entityName];
        entity.canBeImplicitPossessiveOwner = canBeImplicitPossessiveOwner;
        if (!contains(entity.roles, roleName)) {
            entity.roles.push_back(roleName);
        }
        for (const auto& attribute: attributes) {
            if (!contains(entity.attributes, attribute)) {
                entity.attributes.push_back(attribute);
            }
        }

        auto& group = program.roleGroups[roleName];
        group.roleName = roleName;
        if (!contains(group.members, entityName)) {
            group.members.push_back(entityName);
        }
    }

    std::string SemanticAnalyzer::resolveEntityName(Program& program, const hyh::hebrew::NumberExpressionIr& ir,
                                                    std::size_t line) const {
        const std::string fallback = ir.entityName.empty() ? ir.entityHeadLemma : ir.entityName;
        if (fallback.empty()) {
            return fallback;
        }

        DiscourseResolver resolver(program);
        const auto result = resolver.resolveEntity(EntityMentionQuery{
            .surface = ir.entitySurface,
            .headLemma = ir.entityHeadLemma.empty() ? fallback : ir.entityHeadLemma,
            .roleLemma = {},
            .definite = ir.entityDefinite,
            .possessive = false,
            .gender = ir.entityGender,
            .number = ir.entityNumber,
            .line = line,
            .context = MentionContext::PropertyOwner,
            .currentActorName = {},
        });

        switch (result.kind) {
        case ResolutionResult::Kind::Resolved:
            registerEntity(program, result.entityName, line);
            return result.entityName;
        case ResolutionResult::Kind::Ambiguous:
            program.diagnostics.push_back(Diagnostic{"HN010", line, ambiguityMessage(ir, result)});
            return fallback;
        case ResolutionResult::Kind::Unresolved:
            if (ir.entityDefinite) {
                program.diagnostics.push_back(Diagnostic{"HN011", line,
                                                         "unknown definite reference: '"
                                                             + (ir.entitySurface.empty() ? fallback : ir.entitySurface)
                                                             + "'"});
            }
            registerEntity(program, fallback, line, ir.entityGender, ir.entityNumber);
            return fallback;
        }
        return fallback;
    }

    NumberExpr SemanticAnalyzer::convertNumberExpr(Program& program, const hyh::hebrew::NumberExpressionIr& ir,
                                                   std::size_t line, const std::string& currentEntityName) const {
        NumberExpr expr;
        switch (ir.kind) {
        case hyh::hebrew::NumberExpressionIr::Kind::Literal:
            expr.kind = NumberExpr::Kind::Literal;
            expr.literal = ir.literal;
            expr.rawSurface = ir.rawSurface;
            break;
        case hyh::hebrew::NumberExpressionIr::Kind::CurrentActorProperty:
            if (!currentEntityName.empty()) {
                expr.kind = NumberExpr::Kind::EntityProperty;
                expr.entityName = currentEntityName;
            } else {
                expr.kind = NumberExpr::Kind::CurrentActorProperty;
            }
            expr.propertyName = ir.propertyName;
            expr.rawSurface = ir.rawSurface;
            break;
        case hyh::hebrew::NumberExpressionIr::Kind::EntityProperty:
            expr.kind = NumberExpr::Kind::EntityProperty;
            expr.entityName = resolveEntityName(program, ir, line);
            expr.propertyName = ir.propertyName;
            expr.rawSurface = ir.rawSurface;
            break;
        case hyh::hebrew::NumberExpressionIr::Kind::Unknown:
            expr.kind = NumberExpr::Kind::Literal;
            expr.rawSurface = ir.rawSurface;
            break;
        }
        return expr;
    }

    Statement SemanticAnalyzer::lowerSentence(const SemanticInputNode& node, Program& program,
                                              const std::string& currentEntityName) const {
        const auto& sentence = node.sentence;
        Statement statement;
        statement.line = sentence.line;
        statement.rawText = sentence.rawText;

        for (const auto& diagnostic: sentence.diagnostics) {
            const auto separator = diagnostic.find(':');
            const bool hasCode = separator != std::string::npos && separator >= 4 && diagnostic.starts_with("HN");
            program.diagnostics.push_back(Diagnostic{hasCode ? diagnostic.substr(0, separator) : "HN004", sentence.line,
                                                     diagnostic});
        }

        if (auto question = recognizeMirrorQuestion(sentence)) {
            program.mirrorInputsBySlot[question->slotName] =
                MirrorInput{question->prompt, question->slotName, question->answerTypeHint, AnswerType::Unknown,
                            sentence.line};
            statement.kind = Statement::Kind::Input;
            statement.inputPrompt = question->prompt;
            statement.mirrorSlotName = question->slotName;
            statement.mirrorAnswerTypeHint = question->answerTypeHint;
            statement.mirrorAnswerType = AnswerType::Unknown;
            return statement;
        }

        switch (sentence.kind) {
        case hyh::hebrew::HebrewSentenceKind::Import:
            statement.kind = Statement::Kind::Import;
            statement.importedName = sentence.importedName;
            statement.importAlias = aliasKey(sentence.importAlias);
            if (!statement.importAlias.empty() && !statement.importedName.empty()) {
                program.importsByAlias[statement.importAlias] =
                    ImportBinding{statement.importAlias, statement.importedName};
            }
            break;
        case hyh::hebrew::HebrewSentenceKind::RoleIntroduction:
            statement.kind = Statement::Kind::RoleIntroduction;
            statement.entityName = sentence.subjectName;
            statement.roleName = sentence.role.headLemma;
            statement.attributes = sentence.attributes;
            registerRole(program, statement.entityName, statement.roleName, statement.attributes, sentence.line,
                         sentence.role.gender, sentence.role.number, sentence.role.isAnimateCandidate);
            break;
        case hyh::hebrew::HebrewSentenceKind::PropertyAssignment:
            statement.kind = Statement::Kind::PropertyAssignment;
            statement.target = convertNumberExpr(program, sentence.leftNumber, sentence.line, currentEntityName);
            statement.value.kind = NumberExpr::Kind::Literal;
            statement.value.literal = sentence.literalValue;
            if (statement.target.kind == NumberExpr::Kind::EntityProperty && !statement.target.entityName.empty()
                && !statement.target.propertyName.empty()) {
                program.entities[statement.target.entityName].numberProperties[statement.target.propertyName] =
                    statement.value.literal;
            }
            break;
        case hyh::hebrew::HebrewSentenceKind::ForEachRole:
            statement.kind = Statement::Kind::ForEachRole;
            statement.roleName = sentence.role.headLemma;
            break;
        case hyh::hebrew::HebrewSentenceKind::Condition:
            statement.kind = Statement::Kind::Condition;
            statement.left = convertNumberExpr(program, sentence.leftNumber, sentence.line, currentEntityName);
            statement.right = convertNumberExpr(program, sentence.rightNumber, sentence.line, currentEntityName);
            break;
        case hyh::hebrew::HebrewSentenceKind::Speech:
            if (const auto* import = findImport(program, sentence.speakerName)) {
                if (!isDefiniteReference(sentence.speakerName)) {
                    program.diagnostics.push_back(Diagnostic{"HN012", sentence.line,
                                                             "imported callable alias must be referenced definitely: '"
                                                                 + sentence.speakerName + "'"});
                    statement.kind = Statement::Kind::NarrativeEvent;
                    break;
                }

                statement.kind = Statement::Kind::Call;
                statement.calleeName = aliasKey(sentence.speakerName);
                statement.resolvedCalleeName = import->originalName;
                statement.functionName = sentence.predicate.lemma;
                statement.arguments.push_back(sentence.speechText);
            } else {
                statement.kind = Statement::Kind::Speech;
                statement.speakerName = sentence.speakerName;
                statement.speechText = sentence.speechText;
                registerEntity(program, sentence.speakerName, sentence.line);
            }
            break;
        case hyh::hebrew::HebrewSentenceKind::NarrativeEvent:
        case hyh::hebrew::HebrewSentenceKind::TeachingDirective:
        case hyh::hebrew::HebrewSentenceKind::Unknown:
            if (looksLikeBareImportedCall(program, sentence)) {
                program.diagnostics.push_back(Diagnostic{"HN013", sentence.line,
                                                         "imported callable alias must be referenced definitely"});
            }
            statement.kind = Statement::Kind::NarrativeEvent;
            break;
        }
        return statement;
    }

} // namespace hyh::fairy
