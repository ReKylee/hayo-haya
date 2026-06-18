#include "fairy/mirror_type_resolver.hpp"

#include <sstream>
#include <string_view>

namespace hyh::fairy {
    namespace {

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
            return "Unknown";
        }

        std::string expectedValueFromExpr(const NumberExpr& expr) {
            if (!expr.rawSurface.empty()) {
                return expr.rawSurface;
            }
            if (expr.kind == NumberExpr::Kind::Literal) {
                return std::to_string(expr.literal);
            }
            return expr.propertyName.empty() ? expr.entityName : expr.propertyName;
        }

        AnswerType typeFromExprUse(const NumberExpr& expr) {
            return expr.kind == NumberExpr::Kind::Literal ? AnswerType::Number : AnswerType::String;
        }

        void constrain(Program& program, MirrorInput& input, AnswerType type, std::size_t line,
                       std::string_view reason) {
            if (type == AnswerType::Unknown) {
                return;
            }
            if (input.answerType == AnswerType::Unknown) {
                input.answerType = type;
                return;
            }
            if (input.answerType == type) {
                return;
            }

            std::ostringstream message;
            message << "mirror answer type conflict for '" << input.slotName
                    << "': " << answerTypeName(input.answerType) << " vs " << answerTypeName(type) << " from "
                    << reason;
            program.diagnostics.push_back(Diagnostic{"HN020", line, message.str()});
        }

        void constrainFromStatement(Program& program, Statement& statement) {
            if (statement.kind != Statement::Kind::Condition && statement.kind != Statement::Kind::PropertyAssignment) {
                return;
            }

            auto input = program.mirrorInputsBySlot.find(statement.left.rawSurface);
            if (input != program.mirrorInputsBySlot.end()) {
                constrain(program, input->second, typeFromExprUse(statement.right), statement.line, "condition");
                statement.isMirrorCondition = true;
                statement.mirrorSlotName = input->second.slotName;
                statement.mirrorExpectedValue = expectedValueFromExpr(statement.right);
                return;
            }

            input = program.mirrorInputsBySlot.find(statement.target.rawSurface);
            if (input != program.mirrorInputsBySlot.end()) {
                constrain(program, input->second, AnswerType::Number, statement.line, "numeric property assignment");
            }
        }

        void collectConstraints(Program& program, std::vector<Statement>& statements) {
            for (auto& statement: statements) {
                constrainFromStatement(program, statement);
                collectConstraints(program, statement.body);
            }
        }

        void applyResolvedTypes(const Program& program, std::vector<Statement>& statements) {
            for (auto& statement: statements) {
                if (!statement.mirrorSlotName.empty()) {
                    if (const auto input = program.mirrorInputsBySlot.find(statement.mirrorSlotName);
                        input != program.mirrorInputsBySlot.end()) {
                        statement.mirrorAnswerType = input->second.answerType;
                    }
                }
                applyResolvedTypes(program, statement.body);
            }
        }

    } // namespace

    void MirrorTypeResolver::resolve(Program& program) const {
        for (auto& [_, input]: program.mirrorInputsBySlot) {
            input.answerType = input.answerTypeHint;
        }

        collectConstraints(program, program.statements);

        for (auto& [_, input]: program.mirrorInputsBySlot) {
            if (input.answerType == AnswerType::Unknown) {
                input.answerType = AnswerType::String;
            }
        }

        applyResolvedTypes(program, program.statements);
    }

} // namespace hyh::fairy
