#include "codegen.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <type_traits>

namespace hyh {

std::string CodeGenerator::symbolFor(const std::string& hebrewName) {
    auto it = symbols_.find(hebrewName);
    if (it != symbols_.end()) {
        return it->second;
    }

    std::string created = "hyh_" + std::to_string(nextId_++);
    symbols_.emplace(hebrewName, created);
    return created;
}

std::string CodeGenerator::countSymbolFor(const std::string& container, const std::string& unit) {
    (void)unit;
    return symbolFor(container + "::count");
}

std::string CodeGenerator::boolSymbolFor(const std::string& object) {
    return symbolFor(object + "::open");
}

std::string CodeGenerator::textSymbolFor(const std::string& object) {
    return symbolFor(object + "::text");
}

std::string CodeGenerator::escapeString(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (char c : value) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

bool CodeGenerator::isClosedState(const std::string& state) {
    return state == "סגור" || state == "סגורה" || state == "כבוי" || state == "כבויה";
}

bool CodeGenerator::isOpenState(const std::string& state) {
    return state == "פתוח" || state == "פתוחה" || state == "דולק" || state == "דולקת";
}

void CodeGenerator::emitStatements(
    const std::vector<Statement>& statements,
    const std::unordered_map<std::string, std::string>& aliasToSymbol,
    std::ostream& out,
    int indent
) {
    for (const auto& statement : statements) {
        emitStatement(statement, aliasToSymbol, out, indent);
    }
}

void CodeGenerator::emitStatement(
    const Statement& statement,
    const std::unordered_map<std::string, std::string>& aliasToSymbol,
    std::ostream& out,
    int indent
) {
    const std::string spaces(static_cast<std::size_t>(indent), ' ');

    std::visit([&](const auto& s) {
        using T = std::decay_t<decltype(s)>;

        auto emitValue = [&](const std::string& alias, const std::string& cppExpr) {
            auto imported = aliasToSymbol.find(alias);
            const std::string symbol = imported == aliasToSymbol.end() ? "cout" : imported->second;

            if (symbol == "println") {
                out << spaces << "std::println(\"{}\", " << cppExpr << ");\n";
            } else if (symbol == "print") {
                out << spaces << "std::print(\"{}\\n\", " << cppExpr << ");\n";
            } else {
                out << spaces << "std::cout << " << cppExpr << " << '\\n';\n";
            }
        };

        if constexpr (std::is_same_v<T, PrintStringStmt>) {
            emitValue(s.alias, "\"" + escapeString(s.value) + "\"");
        } else if constexpr (std::is_same_v<T, PrintWrittenTextStmt>) {
            emitValue(s.alias, textSymbolFor(s.object));
        } else if constexpr (std::is_same_v<T, CountChangeStmt>) {
            out << spaces << countSymbolFor(s.container, s.unit) << " += " << s.amount << ";\n";
        } else if constexpr (std::is_same_v<T, LoopStmt>) {
            out << spaces << "while (" << countSymbolFor(s.condition.container, s.condition.unit)
                << " < " << s.condition.amount << ") {\n";
            emitStatements(s.body, aliasToSymbol, out, indent + 4);
            out << spaces << "}\n";
        } else if constexpr (std::is_same_v<T, ConditionStmt>) {
            out << spaces << "if (" << countSymbolFor(s.condition.container, s.condition.unit)
                << " < " << s.condition.amount << ") {\n";
            emitStatements(s.body, aliasToSymbol, out, indent + 4);
            out << spaces << "}\n";
        } else if constexpr (std::is_same_v<T, UtteranceConditionStmt>) {
            out << spaces << "if (true) {\n";
            emitStatements(s.body, aliasToSymbol, out, indent + 4);
            out << spaces << "}\n";
        } else if constexpr (std::is_same_v<T, NarrativeStmt>) {
            out << spaces << "// narrative event\n";
        }
    }, statement.value);
}

void CodeGenerator::generate(const Program& program, std::ostream& out) {
    bool usesCout = false;
    bool usesPrint = false;

    std::unordered_map<std::string, std::string> aliasToSymbol;
    for (const auto& import : program.imports) {
        aliasToSymbol[import.alias] = import.symbol;
        if (import.symbol == "cout") {
            usesCout = true;
        }
        if (import.symbol == "print" || import.symbol == "println") {
            usesPrint = true;
        }
    }

    if (usesCout || program.imports.empty()) {
        out << "#include <iostream>\n";
    }
    if (usesPrint) {
        out << "#include <print>\n";
    }
    out << "\n";

    const std::string countrySymbol = symbolFor(program.country.empty() ? "ארץ" : program.country);
    const std::string kingdomSymbol = symbolFor(program.kingdom.empty() ? "ממלכה" : program.kingdom);

    out << "namespace " << countrySymbol << " { // " << program.country << "\n";
    out << "namespace " << kingdomSymbol << " { // " << program.kingdom << "\n\n";

    for (const auto& decl : program.decls) {
        std::visit([&](const auto& d) {
            using T = std::decay_t<decltype(d)>;
            if constexpr (std::is_same_v<T, BoolDecl>) {
                const bool value = isOpenState(d.state) ? true : !isClosedState(d.state);
                out << "bool " << boolSymbolFor(d.object) << " = " << (value ? "true" : "false")
                    << "; // " << d.object << " היה/הייתה " << d.state << "\n";
            } else if constexpr (std::is_same_v<T, TextDecl>) {
                out << "const char* " << textSymbolFor(d.object) << " = \"" << escapeString(d.value)
                    << "\"; // הכתוב שעל " << d.object << "\n";
            } else if constexpr (std::is_same_v<T, CountDecl>) {
                out << "int " << countSymbolFor(d.container, d.unit) << " = " << d.amount
                    << "; // ב" << d.container << " נחו " << d.amount << " " << d.unit << "\n";
            }
        }, decl);
    }

    out << "\nvoid story() {\n";
    emitStatements(program.stmts, aliasToSymbol, out, 4);
    out << "}\n\n";

    out << "} // namespace " << kingdomSymbol << "\n";
    out << "} // namespace " << countrySymbol << "\n\n";
    out << "int main() {\n";
    out << "    " << countrySymbol << "::" << kingdomSymbol << "::story();\n";
    out << "    return 0;\n";
    out << "}\n";
}

} // namespace hyh
