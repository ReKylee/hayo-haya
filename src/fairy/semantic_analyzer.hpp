#pragma once

#include <string>
#include <vector>

#include "fairy/story_ast.hpp"
#include "hebrew/sentence_ir.hpp"

namespace hyh::fairy {

    struct SemanticInputNode {
        hyh::hebrew::HebrewSentence sentence;
        std::vector<SemanticInputNode> children;
    };

    class SemanticAnalyzer {
    public:
        Program analyze(const std::vector<SemanticInputNode>& nodes) const;

    private:
        void lowerNodes(const std::vector<SemanticInputNode>& nodes, Program& program, std::vector<Statement>& out,
                        std::string currentEntityName = {}) const;
        Statement lowerSentence(const SemanticInputNode& node, Program& program,
                                const std::string& currentEntityName) const;

        void registerEntity(Program& program, const std::string& entityName, std::size_t line,
                            hyh::hebrew::Gender gender = hyh::hebrew::Gender::Unknown,
                            hyh::hebrew::Number number = hyh::hebrew::Number::Unknown) const;

        void registerRole(Program& program, const std::string& entityName, const std::string& roleName,
                          const std::vector<std::string>& attributes, std::size_t line, hyh::hebrew::Gender gender,
                          hyh::hebrew::Number number, bool canBeImplicitPossessiveOwner) const;

        NumberExpr convertNumberExpr(Program& program, const hyh::hebrew::NumberExpressionIr& ir, std::size_t line,
                                     const std::string& currentEntityName) const;
        std::string resolveEntityName(Program& program, const hyh::hebrew::NumberExpressionIr& ir,
                                      std::size_t line) const;
    };

} // namespace hyh::fairy
