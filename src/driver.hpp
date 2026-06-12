#pragma once

#include <optional>
#include <string>

#include "ast.hpp"
#include "lexer.hpp"

namespace hyh {

class Driver {
public:
    explicit Driver(std::string source);

    Parser::symbol_type lex();
    int parse();

    void reportError(const Parser::location_type& location, const std::string& message);

    void setCountry(std::string name);
    void setKingdom(std::string name);

    void addStdImport(std::string alias, std::string symbol);
    void addBoolDecl(std::string object, std::string state);
    void addTextDecl(std::string object, std::string value);
    void addCountDecl(std::string container, int amount, std::string unit);

    void addPrintString(std::string alias, std::string value);
    void addPrintWrittenText(std::string alias, std::string object);
    void addStatement(Statement statement);

    Statement makePrintString(std::string alias, std::string value) const;
    Statement makePrintWrittenText(std::string alias, std::string object) const;
    Statement makeCountChange(std::string container, std::string unit, int amount) const;
    Statement makeLoop(CountCondition condition, std::vector<Statement> body) const;
    Statement makeCondition(CountCondition condition, std::vector<Statement> body) const;
    Statement makeUtteranceCondition(std::vector<Statement> body) const;
    Statement makeNarrative() const;
    CountCondition makeCountCondition(std::string container, std::string unit, int amount) const;

    void finish(std::string kingdomName);

    std::string stripOptionalDefinite(std::string text) const;
    std::string stripRequiredPrefix(std::string text, const std::string& utf8Prefix, const std::string& description) const;

    const Program& program() const;
    bool hadError() const;

private:
    Lexer lexer_;
    Program program_;
    bool hadError_ = false;
};

} // namespace hyh
