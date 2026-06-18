#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "hebrew/features.hpp"

namespace hyh::fairy {

    struct Diagnostic {
        std::string code;
        std::size_t line = 1;
        std::string message;
    };

    struct Entity {
        std::string name;
        std::string headLemma;
        std::vector<std::string> roles;
        std::vector<std::string> attributes;
        std::unordered_map<std::string, int> numberProperties;
        hyh::hebrew::Gender gender = hyh::hebrew::Gender::Unknown;
        hyh::hebrew::Number number = hyh::hebrew::Number::Unknown;
        bool canBeImplicitPossessiveOwner = true;
        std::size_t firstMentionLine = 0;
        std::size_t lastMentionLine = 0;
        std::size_t mentionCount = 0;
    };

    struct RoleGroup {
        std::string roleName;
        std::vector<std::string> members;
    };

    struct ImportBinding {
        std::string aliasName;
        std::string originalName;
    };

    struct NumberExpr {
        enum class Kind {
            Literal,
            CurrentActorProperty,
            EntityProperty
        };

        Kind kind = Kind::Literal;
        int literal = 0;
        std::string entityName;
        std::string propertyName;
        std::string rawSurface;
    };

    enum class AnswerType {
        Unknown,
        String,
        Number,
        Boolean
    };

    struct MirrorInput {
        std::string prompt;
        std::string slotName;
        AnswerType answerTypeHint = AnswerType::Unknown;
        AnswerType answerType = AnswerType::String;
        std::size_t line = 1;
    };

    struct Statement {
        enum class Kind {
            Import,
            RoleIntroduction,
            PropertyAssignment,
            ForEachRole,
            Condition,
            Call,
            Speech,
            Input,
            NarrativeEvent
        };

        Kind kind = Kind::NarrativeEvent;
        std::size_t line = 1;
        std::string rawText;

        std::string entityName;
        std::string roleName;
        std::vector<std::string> attributes;

        NumberExpr target;
        NumberExpr value;
        NumberExpr left;
        NumberExpr right;
        bool isMirrorCondition = false;
        std::string mirrorSlotName;
        std::string mirrorExpectedValue;
        AnswerType mirrorAnswerTypeHint = AnswerType::Unknown;
        AnswerType mirrorAnswerType = AnswerType::String;

        std::string speakerName;
        std::string speechText;
        std::string inputPrompt;

        std::string importedName;
        std::string importAlias;
        std::string calleeName;
        std::string resolvedCalleeName;
        std::string functionName;
        std::vector<std::string> arguments;

        std::vector<Statement> body;
    };

    struct Program {
        std::unordered_map<std::string, Entity> entities;
        std::unordered_map<std::string, RoleGroup> roleGroups;
        std::unordered_map<std::string, ImportBinding> importsByAlias;
        std::unordered_map<std::string, MirrorInput> mirrorInputsBySlot;
        std::vector<Statement> statements;
        std::vector<Diagnostic> diagnostics;
    };

} // namespace hyh::fairy
