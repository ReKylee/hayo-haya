#include "driver.hpp"

#include <iostream>
#include <stdexcept>
#include <utility>

#include "parser.hpp"

namespace hyh {

Driver::Driver(std::string source)
    : lexer_(std::move(source)) {}

Parser::symbol_type Driver::lex() {
    return lexer_.next();
}

int Driver::parse() {
    Parser parser(*this);
    return parser.parse();
}

void Driver::reportError(const Parser::location_type& location, const std::string& message) {
    hadError_ = true;
    std::cerr << "שיבוש בסיפור";
    if (location.begin.line > 0) {
        std::cerr << " near line " << location.begin.line;
    }
    std::cerr << ": " << message << '\n';
}

void Driver::setCountry(std::string name) {
    program_.country = std::move(name);
}

void Driver::setKingdom(std::string name) {
    program_.kingdom = std::move(name);
}

void Driver::addStdImport(std::string alias, std::string symbol) {
    program_.imports.push_back(ImportDecl{
        .alias = std::move(alias),
        .source = "הממלכה העתיקה",
        .symbol = std::move(symbol),
    });
}

void Driver::addBoolDecl(std::string object, std::string state) {
    program_.decls.push_back(BoolDecl{
        .object = std::move(object),
        .state = std::move(state),
    });
}

void Driver::addTextDecl(std::string object, std::string value) {
    program_.decls.push_back(TextDecl{
        .object = std::move(object),
        .value = std::move(value),
    });
}

void Driver::addCountDecl(std::string container, int amount, std::string unit) {
    program_.decls.push_back(CountDecl{
        .container = std::move(container),
        .amount = amount,
        .unit = std::move(unit),
    });
}

void Driver::addPrintString(std::string alias, std::string value) {
    program_.stmts.push_back(makePrintString(std::move(alias), std::move(value)));
}

void Driver::addPrintWrittenText(std::string alias, std::string object) {
    program_.stmts.push_back(makePrintWrittenText(std::move(alias), std::move(object)));
}

void Driver::addStatement(Statement statement) {
    program_.stmts.push_back(std::move(statement));
}

Statement Driver::makePrintString(std::string alias, std::string value) const {
    return Statement{PrintStringStmt{
        .alias = std::move(alias),
        .value = std::move(value),
    }};
}

Statement Driver::makePrintWrittenText(std::string alias, std::string object) const {
    return Statement{PrintWrittenTextStmt{
        .alias = std::move(alias),
        .object = std::move(object),
    }};
}

Statement Driver::makeCountChange(std::string container, std::string unit, int amount) const {
    return Statement{CountChangeStmt{
        .container = std::move(container),
        .unit = std::move(unit),
        .amount = amount,
    }};
}

Statement Driver::makeLoop(CountCondition condition, std::vector<Statement> body) const {
    return Statement{LoopStmt{
        .condition = std::move(condition),
        .body = std::move(body),
    }};
}

Statement Driver::makeCondition(CountCondition condition, std::vector<Statement> body) const {
    return Statement{ConditionStmt{
        .condition = std::move(condition),
        .body = std::move(body),
    }};
}

Statement Driver::makeUtteranceCondition(std::vector<Statement> body) const {
    return Statement{UtteranceConditionStmt{
        .body = std::move(body),
    }};
}

Statement Driver::makeNarrative() const {
    return Statement{NarrativeStmt{}};
}

CountCondition Driver::makeCountCondition(std::string container, std::string unit, int amount) const {
    return CountCondition{
        .container = std::move(container),
        .unit = std::move(unit),
        .amount = amount,
    };
}

void Driver::finish(std::string kingdomName) {
    if (!program_.kingdom.empty() && kingdomName != program_.kingdom) {
        hadError_ = true;
        std::cerr << "שיבוש בסיפור: הסיפור נפתח בממלכת " << program_.kingdom
                  << " אך הסתיים בממלכת " << kingdomName << "\n";
    }
}

std::string Driver::stripOptionalDefinite(std::string text) const {
    // Hebrew ה is two bytes in UTF-8. Keywords are recognized before NAME tokens,
    // so this is only applied to identifiers such as הכרוז or השער.
    static const std::string he = "ה";
    if (text.rfind(he, 0) == 0 && text.size() > he.size()) {
        return text.substr(he.size());
    }
    return text;
}

std::string Driver::stripRequiredPrefix(std::string text, const std::string& utf8Prefix, const std::string& description) const {
    if (text.rfind(utf8Prefix, 0) != 0 || text.size() <= utf8Prefix.size()) {
        throw std::runtime_error("Expected " + description + " to start with prefix '" + utf8Prefix + "', got: " + text);
    }
    return text.substr(utf8Prefix.size());
}

const Program& Driver::program() const {
    return program_;
}

bool Driver::hadError() const {
    return hadError_;
}

} // namespace hyh
