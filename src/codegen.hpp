#pragma once

#include <ostream>
#include <string>
#include <unordered_map>

#include "ast.hpp"

namespace hyh {

class CodeGenerator {
public:
    void generate(const Program& program, std::ostream& out);

private:
    std::unordered_map<std::string, std::string> symbols_;
    int nextId_ = 1;

    void emitStatements(const std::vector<Statement>& statements, const std::unordered_map<std::string, std::string>& aliasToSymbol, std::ostream& out, int indent);
    void emitStatement(const Statement& statement, const std::unordered_map<std::string, std::string>& aliasToSymbol, std::ostream& out, int indent);

    std::string symbolFor(const std::string& hebrewName);
    std::string countSymbolFor(const std::string& container, const std::string& unit);
    std::string boolSymbolFor(const std::string& object);
    std::string textSymbolFor(const std::string& object);

    static std::string escapeString(const std::string& value);
    static bool isClosedState(const std::string& state);
    static bool isOpenState(const std::string& state);
};

} // namespace hyh
