#include "fairy/semantic_dump.hpp"

#include <ostream>
#include <string>
#include <string_view>

#include "hebrew/features.hpp"

namespace hyh::fairy {
    namespace {

        std::string statementKindName(Statement::Kind kind) {
            switch (kind) {
            case Statement::Kind::Import:
                return "Import";
            case Statement::Kind::RoleIntroduction:
                return "RoleIntroduction";
            case Statement::Kind::PropertyAssignment:
                return "PropertyAssignment";
            case Statement::Kind::ForEachRole:
                return "ForEachRole";
            case Statement::Kind::Condition:
                return "Condition";
            case Statement::Kind::Call:
                return "Call";
            case Statement::Kind::Speech:
                return "Speech";
            case Statement::Kind::Input:
                return "Input";
            case Statement::Kind::NarrativeEvent:
                return "NarrativeEvent";
            }
            return "NarrativeEvent";
        }

        std::string answerTypeName(AnswerType type) {
            switch (type) {
            case AnswerType::Unknown:
                return "Unknown";
            case AnswerType::String:
                return "String";
            case AnswerType::Number:
                return "Number";
            case AnswerType::Boolean:
                return "Boolean";
            }
            return "String";
        }

        int numberKindId(NumberExpr::Kind kind) {
            switch (kind) {
            case NumberExpr::Kind::Literal:
                return 1;
            case NumberExpr::Kind::CurrentActorProperty:
                return 2;
            case NumberExpr::Kind::EntityProperty:
                return 3;
            }
            return 0;
        }

        std::string ownerName(const NumberExpr& expr) {
            switch (expr.kind) {
            case NumberExpr::Kind::CurrentActorProperty:
                return "<current-actor>";
            case NumberExpr::Kind::EntityProperty:
                return expr.entityName.empty() ? "<unknown-entity>" : expr.entityName;
            case NumberExpr::Kind::Literal:
                return "<literal>";
            }
            return "<unknown>";
        }

        void dumpNumberExpr(std::ostream& out, std::string_view label, const NumberExpr& expr) {
            if (expr.kind == NumberExpr::Kind::Literal) {
                out << "  " << label << "-number: kind=" << numberKindId(expr.kind) << " value=" << expr.literal
                    << '\n';
                return;
            }

            if (expr.propertyName.empty()) {
                return;
            }

            out << "  " << label << "-number: kind=" << numberKindId(expr.kind) << " property=" << expr.propertyName
                << " owner=" << ownerName(expr) << '\n';
        }

        void dumpStatements(std::ostream& out, const Program& program, const std::vector<Statement>& statements) {
            for (const auto& statement: statements) {
                out << statement.line << ": " << statementKindName(statement.kind) << " :: " << statement.rawText
                    << '\n';

                switch (statement.kind) {
                case Statement::Kind::Import:
                    out << "  import: " << statement.importedName << " as " << statement.importAlias << '\n';
                    break;
                case Statement::Kind::RoleIntroduction: {
                    out << "  subject/name: " << statement.entityName << '\n';
                    out << "  role/head: " << statement.roleName;
                    const auto entityIt = program.entities.find(statement.entityName);
                    if (entityIt != program.entities.end()) {
                        out << " gender=" << hebrew::toString(entityIt->second.gender)
                            << " number=" << hebrew::toString(entityIt->second.number);
                    }
                    out << '\n';
                    break;
                }
                case Statement::Kind::PropertyAssignment:
                    dumpNumberExpr(out, "left", statement.target);
                    break;
                case Statement::Kind::ForEachRole: {
                    out << "  role/head: " << statement.roleName;
                    const auto groupIt = program.roleGroups.find(statement.roleName);
                    if (groupIt != program.roleGroups.end() && !groupIt->second.members.empty()) {
                        const auto entityIt = program.entities.find(groupIt->second.members.front());
                        if (entityIt != program.entities.end()) {
                            out << " gender=" << hebrew::toString(entityIt->second.gender)
                                << " number=" << hebrew::toString(entityIt->second.number);
                        }
                    }
                    out << '\n';
                    break;
                }
                case Statement::Kind::Condition:
                    if (statement.isMirrorCondition) {
                        out << "  mirror-slot: " << statement.mirrorSlotName << '\n';
                        out << "  answer-type: " << answerTypeName(statement.mirrorAnswerType) << '\n';
                        out << "  expected: " << statement.mirrorExpectedValue << '\n';
                    } else {
                        dumpNumberExpr(out, "left", statement.left);
                        dumpNumberExpr(out, "right", statement.right);
                    }
                    break;
                case Statement::Kind::Call:
                    out << "  callee/name: " << statement.calleeName << '\n';
                    out << "  callee/resolved: " << statement.resolvedCalleeName << '\n';
                    out << "  function: " << statement.functionName << '\n';
                    for (const auto& argument: statement.arguments) {
                        out << "  argument/string: " << argument << '\n';
                    }
                    break;
                case Statement::Kind::Speech:
                    out << "  speaker/name: " << statement.speakerName << '\n';
                    out << "  speech: " << statement.speechText << '\n';
                    break;
                case Statement::Kind::Input:
                    out << "  prompt: " << statement.inputPrompt << '\n';
                    out << "  mirror-slot: " << statement.mirrorSlotName << '\n';
                    out << "  answer-type: " << answerTypeName(statement.mirrorAnswerType) << '\n';
                    break;
                case Statement::Kind::NarrativeEvent:
                    break;
                }

                dumpStatements(out, program, statement.body);
            }
        }

    } // namespace

    void dumpSemanticIr(std::ostream& out, const Program& program) {
        dumpStatements(out, program, program.statements);
    }

} // namespace hyh::fairy
