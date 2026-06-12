#pragma once

#include <string>
#include <variant>
#include <vector>

namespace hyh {

struct ImportDecl {
    std::string alias;      // Hebrew story name, e.g. כרוז
    std::string source;     // For now: הממלכה העתיקה
    std::string symbol;     // External C++ name, e.g. cout
};

struct BoolDecl {
    std::string object;
    std::string state;
};

struct TextDecl {
    std::string object;
    std::string value;
};

struct CountDecl {
    std::string container;
    int amount = 0;
    std::string unit;
};

using Decl = std::variant<BoolDecl, TextDecl, CountDecl>;

struct PrintStringStmt {
    std::string alias;
    std::string value;
};

struct PrintWrittenTextStmt {
    std::string alias;
    std::string object;
};

struct CountChangeStmt {
    std::string container;
    std::string unit;
    int amount = 0;
};

struct CountCondition {
    std::string container;
    std::string unit;
    int amount = 0;
};

struct NarrativeStmt {};

struct Statement;

struct LoopStmt {
    CountCondition condition;
    std::vector<Statement> body;
};

struct ConditionStmt {
    CountCondition condition;
    std::vector<Statement> body;
};

struct UtteranceConditionStmt {
    std::vector<Statement> body;
};

using Stmt = std::variant<
    PrintStringStmt,
    PrintWrittenTextStmt,
    CountChangeStmt,
    LoopStmt,
    ConditionStmt,
    UtteranceConditionStmt,
    NarrativeStmt
>;

struct Statement {
    Stmt value;
};

struct Program {
    std::string country;
    std::string kingdom;
    std::vector<ImportDecl> imports;
    std::vector<Decl> decls;
    std::vector<Statement> stmts;
};

} // namespace hyh
