#include "codegen/cpp_codegen.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <map>
#include <sstream>
#include <set>
#include <utility>

namespace hyh::codegen {
    namespace {

        std::string escapeCppString(const std::string& value) {
            std::string out;
            for (const char c: value) {
                switch (c) {
                case '\\':
                    out += "\\\\";
                    break;
                case '"':
                    out += "\\\"";
                    break;
                case '\n':
                    out += "\\n";
                    break;
                case '\r':
                    out += "\\r";
                    break;
                case '\t':
                    out += "\\t";
                    break;
                default:
                    out.push_back(c);
                    break;
                }
            }
            return out;
        }

        std::string q(const std::string& value) {
            return "\"" + escapeCppString(value) + "\"";
        }

        std::string indent(int level) {
            return std::string(static_cast<std::size_t>(level) * 4, ' ');
        }

        struct TypeInfo {
            std::string name;
            std::set<std::string> properties;
        };

        struct Symbols {
            std::vector<TypeInfo> types;
            std::map<std::string, std::string> entityTypes;
            std::map<std::string, std::string> entityVars;
            std::map<std::string, std::string> propertyFields;
            std::map<std::string, std::string> roleGroupVars;
            std::map<std::string, std::string> mirrorInputVars;
        };

        using EntityProperties = std::map<std::string, std::set<std::string>>;
        using EntityInitialValues = std::map<std::string, std::map<std::string, int>>;

        struct HoistedInitializers {
            EntityInitialValues values;
            std::set<const hyh::fairy::Statement*> statements;
        };

        std::vector<std::string> sortedEntityNames(const hyh::fairy::Program& program,
                                                   const EntityProperties& properties) {
            std::set<std::string> names;
            for (const auto& [name, _]: program.entities) {
                names.insert(name);
            }
            for (const auto& [name, _]: properties) {
                names.insert(name);
            }
            return {names.begin(), names.end()};
        }

        std::vector<std::string> sortedRoleNames(const hyh::fairy::Program& program) {
            std::vector<std::string> names;
            names.reserve(program.roleGroups.size());
            for (const auto& [name, _]: program.roleGroups) {
                names.push_back(name);
            }
            std::ranges::sort(names);
            return names;
        }

        void collectNumberExpr(const hyh::fairy::NumberExpr& expr, EntityProperties& properties,
                               const std::string& actorName) {
            using Kind = hyh::fairy::NumberExpr::Kind;
            switch (expr.kind) {
            case Kind::EntityProperty:
                if (!expr.entityName.empty() && !expr.propertyName.empty()) {
                    properties[expr.entityName].insert(expr.propertyName);
                }
                break;
            case Kind::CurrentActorProperty:
                if (!actorName.empty() && !expr.propertyName.empty()) {
                    properties[actorName].insert(expr.propertyName);
                }
                break;
            case Kind::Literal:
                break;
            }
        }

        void collectStatements(const hyh::fairy::Program& program,
                               const std::vector<hyh::fairy::Statement>& statements,
                               EntityProperties& properties,
                               const std::string& actorName = {}) {
            for (const auto& statement: statements) {
                switch (statement.kind) {
                case hyh::fairy::Statement::Kind::Import:
                case hyh::fairy::Statement::Kind::Call:
                case hyh::fairy::Statement::Kind::Speech:
                case hyh::fairy::Statement::Kind::Input:
                case hyh::fairy::Statement::Kind::NarrativeEvent:
                    break;
                case hyh::fairy::Statement::Kind::PropertyAssignment:
                    collectNumberExpr(statement.target, properties, actorName);
                    collectNumberExpr(statement.value, properties, actorName);
                    break;
                case hyh::fairy::Statement::Kind::ForEachRole:
                    if (const auto group = program.roleGroups.find(statement.roleName); group != program.roleGroups.end()) {
                        for (const auto& member: group->second.members) {
                            collectStatements(program, statement.body, properties, member);
                        }
                    }
                    break;
                case hyh::fairy::Statement::Kind::Condition:
                    if (!statement.isMirrorCondition) {
                        collectNumberExpr(statement.left, properties, actorName);
                        collectNumberExpr(statement.right, properties, actorName);
                    }
                    collectStatements(program, statement.body, properties, actorName);
                    break;
                case hyh::fairy::Statement::Kind::RoleIntroduction:
                    break;
                }
            }
        }

        bool isLiteralEntityPropertyAssignment(const hyh::fairy::Statement& statement) {
            return statement.kind == hyh::fairy::Statement::Kind::PropertyAssignment
                && statement.target.kind == hyh::fairy::NumberExpr::Kind::EntityProperty
                && statement.value.kind == hyh::fairy::NumberExpr::Kind::Literal
                && !statement.target.entityName.empty()
                && !statement.target.propertyName.empty();
        }

        HoistedInitializers collectHoistedInitializers(const std::vector<hyh::fairy::Statement>& statements) {
            HoistedInitializers hoisted;
            std::set<std::pair<std::string, std::string>> initializedProperties;
            bool inInitializationPhase = true;

            for (const auto& statement: statements) {
                switch (statement.kind) {
                case hyh::fairy::Statement::Kind::Import:
                case hyh::fairy::Statement::Kind::RoleIntroduction:
                case hyh::fairy::Statement::Kind::NarrativeEvent:
                    continue;
                case hyh::fairy::Statement::Kind::PropertyAssignment:
                    if (!inInitializationPhase || !isLiteralEntityPropertyAssignment(statement)) {
                        inInitializationPhase = false;
                        continue;
                    }

                    if (initializedProperties.emplace(statement.target.entityName, statement.target.propertyName)
                            .second) {
                        hoisted.values[statement.target.entityName][statement.target.propertyName] =
                            statement.value.literal;
                        hoisted.statements.insert(&statement);
                    }
                    continue;
                case hyh::fairy::Statement::Kind::ForEachRole:
                case hyh::fairy::Statement::Kind::Condition:
                case hyh::fairy::Statement::Kind::Call:
                case hyh::fairy::Statement::Kind::Speech:
                case hyh::fairy::Statement::Kind::Input:
                    inInitializationPhase = false;
                    continue;
                }
            }

            return hoisted;
        }

        bool isAsciiIdentifierStart(unsigned char c) {
            return std::isalpha(c) != 0 || c == '_';
        }

        bool isAsciiIdentifierPart(unsigned char c) {
            return std::isalnum(c) != 0 || c == '_';
        }

        std::string sanitizedIdentifierBase(const std::string& value) {
            std::string out;
            bool previousWasSeparator = false;
            for (const unsigned char c: value) {
                if (isAsciiIdentifierPart(c)) {
                    out.push_back(static_cast<char>(std::tolower(c)));
                    previousWasSeparator = false;
                    continue;
                }

                if (!out.empty() && !previousWasSeparator && c < 128) {
                    out.push_back('_');
                    previousWasSeparator = true;
                }
            }

            while (!out.empty() && out.back() == '_') {
                out.pop_back();
            }
            if (!out.empty() && !isAsciiIdentifierStart(static_cast<unsigned char>(out.front()))) {
                out.insert(out.begin(), '_');
            }
            return out;
        }

        std::string uniqueIdentifier(const std::string& preferred, const std::set<std::string>& used,
                                     int fallbackIndex) {
            std::string base = preferred.empty() ? "hyh_property_" + std::to_string(fallbackIndex) : preferred;
            if (!used.contains(base)) {
                return base;
            }

            int suffix = 2;
            while (true) {
                const auto candidate = base + "_" + std::to_string(suffix);
                if (!used.contains(candidate)) {
                    return candidate;
                }
                ++suffix;
            }
        }

        Symbols makeSymbols(const hyh::fairy::Program& program, const EntityProperties& properties) {
            Symbols symbols;
            int nextEntity = 1;
            for (const auto& entityName: sortedEntityNames(program, properties)) {
                symbols.entityVars[entityName] = "hyh_entity_" + std::to_string(nextEntity);
                ++nextEntity;
            }

            int nextType = 1;
            for (const auto& roleName: sortedRoleNames(program)) {
                const auto group = program.roleGroups.find(roleName);
                if (group == program.roleGroups.end() || group->second.members.empty()) {
                    continue;
                }

                TypeInfo type;
                type.name = "hyh_role_" + std::to_string(nextType) + "_t";
                symbols.roleGroupVars[roleName] = "hyh_role_group_" + std::to_string(nextType);
                ++nextType;

                for (const auto& entityName: group->second.members) {
                    if (const auto it = properties.find(entityName); it != properties.end()) {
                        type.properties.insert(it->second.begin(), it->second.end());
                    }
                    symbols.entityTypes.emplace(entityName, type.name);
                }
                symbols.types.push_back(std::move(type));
            }

            std::map<std::set<std::string>, std::string> fallbackTypes;
            for (const auto& entityName: sortedEntityNames(program, properties)) {
                if (symbols.entityTypes.contains(entityName)) {
                    continue;
                }

                std::set<std::string> shape;
                if (const auto it = properties.find(entityName); it != properties.end()) {
                    shape = it->second;
                }

                auto [shapeIt, inserted] = fallbackTypes.emplace(shape, "");
                if (inserted) {
                    shapeIt->second = "hyh_entity_shape_" + std::to_string(nextType) + "_t";
                    symbols.types.push_back(TypeInfo{shapeIt->second, shape});
                    ++nextType;
                }
                symbols.entityTypes[entityName] = shapeIt->second;
            }

            int nextProperty = 1;
            std::set<std::string> usedFieldNames;
            std::set<std::string> allPropertyNames;
            for (const auto& [_, propertyNames]: properties) {
                for (const auto& propertyName: propertyNames) {
                    allPropertyNames.insert(propertyName);
                }
            }
            for (const auto& propertyName: allPropertyNames) {
                const auto fieldName = uniqueIdentifier(sanitizedIdentifierBase(propertyName), usedFieldNames, nextProperty);
                symbols.propertyFields[propertyName] = fieldName;
                usedFieldNames.insert(fieldName);
                ++nextProperty;
            }

            std::vector<std::string> mirrorSlots;
            mirrorSlots.reserve(program.mirrorInputsBySlot.size());
            for (const auto& [slotName, _]: program.mirrorInputsBySlot) {
                mirrorSlots.push_back(slotName);
            }
            std::ranges::sort(mirrorSlots);

            int nextInput = 1;
            for (const auto& slotName: mirrorSlots) {
                symbols.mirrorInputVars[slotName] = "hyh_mirror_answer_" + std::to_string(nextInput);
                ++nextInput;
            }
            return symbols;
        }

        std::string entityVar(const Symbols& symbols, const std::string& entityName) {
            const auto it = symbols.entityVars.find(entityName);
            return it == symbols.entityVars.end() ? "" : it->second;
        }

        std::string propertyField(const Symbols& symbols, const std::string& entityName,
                                  const std::string& propertyName) {
            (void)entityName;
            const auto it = symbols.propertyFields.find(propertyName);
            return it == symbols.propertyFields.end() ? "" : it->second;
        }

        std::string entityPropertyExpr(const Symbols& symbols, const std::string& entityName,
                                       const std::string& propertyName) {
            const auto var = entityVar(symbols, entityName);
            const auto field = propertyField(symbols, entityName, propertyName);
            if (var.empty() || field.empty()) {
                return "0 /* unresolved property */";
            }
            return var + "." + field;
        }

        std::string actorPropertyExpr(const Symbols& symbols, const std::string& actorPtrName,
                                      const std::string& propertyName) {
            const auto field = propertyField(symbols, {}, propertyName);
            if (actorPtrName.empty() || field.empty()) {
                return "0 /* unresolved actor property */";
            }
            return actorPtrName + "." + field;
        }

        std::string numberExpr(const hyh::fairy::NumberExpr& expr, const Symbols& symbols,
                               const std::string& actorPtrName) {
            using Kind = hyh::fairy::NumberExpr::Kind;
            switch (expr.kind) {
            case Kind::Literal:
                return std::to_string(expr.literal);
            case Kind::CurrentActorProperty:
                return actorPropertyExpr(symbols, actorPtrName, expr.propertyName);
            case Kind::EntityProperty:
                return entityPropertyExpr(symbols, expr.entityName, expr.propertyName);
            }
            return "0";
        }

        std::string assignmentTarget(const hyh::fairy::NumberExpr& expr, const Symbols& symbols,
                                     const std::string& actorPtrName) {
            using Kind = hyh::fairy::NumberExpr::Kind;
            switch (expr.kind) {
            case Kind::CurrentActorProperty:
                return actorPropertyExpr(symbols, actorPtrName, expr.propertyName);
            case Kind::EntityProperty:
                return entityPropertyExpr(symbols, expr.entityName, expr.propertyName);
            case Kind::Literal:
                return "/* invalid literal assignment target */";
            }
            return "/* invalid assignment target */";
        }

        std::string mirrorInputVar(const Symbols& symbols, const std::string& slotName) {
            const auto it = symbols.mirrorInputVars.find(slotName);
            return it == symbols.mirrorInputVars.end() ? "hyh_missing_mirror_answer" : it->second;
        }

        bool isIntegerLiteral(const std::string& value) {
            if (value.empty()) {
                return false;
            }

            std::size_t begin = value.front() == '-' ? 1 : 0;
            if (begin == value.size()) {
                return false;
            }
            return std::all_of(value.begin() + static_cast<long>(begin), value.end(), [](unsigned char c) {
                return std::isdigit(c) != 0;
            });
        }

        std::string boolLiteralFromHebrew(const std::string& value) {
            return value == "כן" || value == "אמת" || value == "true" || value == "1" ? "true" : "false";
        }

        std::string mirrorExpectedExpr(const hyh::fairy::Statement& statement) {
            switch (statement.mirrorAnswerType) {
            case hyh::fairy::AnswerType::Unknown:
                return q(statement.mirrorExpectedValue);
            case hyh::fairy::AnswerType::Number:
                return isIntegerLiteral(statement.mirrorExpectedValue) ? statement.mirrorExpectedValue : "0";
            case hyh::fairy::AnswerType::Boolean:
                return boolLiteralFromHebrew(statement.mirrorExpectedValue);
            case hyh::fairy::AnswerType::String:
                return q(statement.mirrorExpectedValue);
            }
            return q(statement.mirrorExpectedValue);
        }

        void emitMirrorInput(std::ostringstream& out, const Symbols& symbols, const hyh::fairy::Statement& statement,
                             int level) {
            const auto inputVar = mirrorInputVar(symbols, statement.mirrorSlotName);
            const auto rawVar = inputVar + "_raw";
            out << indent(level) << "std::print(" << q(statement.inputPrompt + " ") << ");\n";
            out << indent(level) << "std::string " << rawVar << ";\n";
            out << indent(level) << "std::getline(std::cin, " << rawVar << ");\n";

            switch (statement.mirrorAnswerType) {
            case hyh::fairy::AnswerType::Unknown:
                out << indent(level) << "std::string " << inputVar << " = " << rawVar << ";\n";
                break;
            case hyh::fairy::AnswerType::Number:
                out << indent(level) << "int " << inputVar << " = std::stoi(" << rawVar << ");\n";
                break;
            case hyh::fairy::AnswerType::Boolean:
                out << indent(level) << "bool " << inputVar << " = " << rawVar << " == \"כן\" || " << rawVar
                    << " == \"אמת\" || " << rawVar << " == \"true\" || " << rawVar << " == \"1\";\n";
                break;
            case hyh::fairy::AnswerType::String:
                out << indent(level) << "std::string " << inputVar << " = " << rawVar << ";\n";
                break;
            }
            out << indent(level) << "(void)" << inputVar << ";\n";
        }

        std::string entityInitializer(const Symbols& symbols, const std::string& entityName,
                                      const EntityInitialValues& initialValues) {
            std::ostringstream out;
            out << "{";

            bool hasPrevious = false;
            if (const auto entityValues = initialValues.find(entityName); entityValues != initialValues.end()) {
                for (const auto& [propertyName, value]: entityValues->second) {
                    const auto field = propertyField(symbols, entityName, propertyName);
                    if (field.empty()) {
                        continue;
                    }

                    if (hasPrevious) {
                        out << ", ";
                    }
                    out << "." << field << " = " << value;
                    hasPrevious = true;
                }
            }

            if (hasPrevious) {
                out << ", ";
            }
            out << ".name = " << q(entityName) << "}";
            return out.str();
        }

        void emitStatements(std::ostringstream& out, const hyh::fairy::Program& program,
                            const std::vector<hyh::fairy::Statement>& statements, const Symbols& symbols, int level,
                            int& nextInputId, const std::string& actorPtrName = {},
                            const std::set<const hyh::fairy::Statement*>* hoistedStatements = nullptr) {
            for (const auto& statement: statements) {
                out << indent(level) << "// " << escapeCppString(statement.rawText) << '\n';
                switch (statement.kind) {
                case hyh::fairy::Statement::Kind::Import:
                    out << indent(level) << "/* import: " << escapeCppString(statement.importedName)
                        << " as " << escapeCppString(statement.importAlias) << " */\n";
                    break;
                case hyh::fairy::Statement::Kind::RoleIntroduction:
                    out << indent(level) << "/* role: " << escapeCppString(statement.roleName) << " */\n";
                    break;
                case hyh::fairy::Statement::Kind::PropertyAssignment:
                    if (hoistedStatements != nullptr && hoistedStatements->contains(&statement)) {
                        out << indent(level) << "/* initialized above */\n";
                        break;
                    }
                    out << indent(level) << assignmentTarget(statement.target, symbols, actorPtrName)
                        << " = " << numberExpr(statement.value, symbols, actorPtrName) << ";\n";
                    break;
                case hyh::fairy::Statement::Kind::ForEachRole:
                    if (const auto group = program.roleGroups.find(statement.roleName); group != program.roleGroups.end()) {
                        if (const auto groupVar = symbols.roleGroupVars.find(statement.roleName);
                            groupVar != symbols.roleGroupVars.end()) {
                            const auto actorVar = "hyh_actor_" + std::to_string(level);
                            out << indent(level) << "for (auto& " << actorVar << " : " << groupVar->second << ") {\n";
                            emitStatements(out, program, statement.body, symbols, level + 1, nextInputId, actorVar);
                            out << indent(level) << "}\n";
                        }
                    }
                    break;
                case hyh::fairy::Statement::Kind::Condition:
                    if (statement.isMirrorCondition) {
                        out << indent(level) << "if (" << mirrorInputVar(symbols, statement.mirrorSlotName)
                            << " == " << mirrorExpectedExpr(statement) << ") {\n";
                    } else {
                        out << indent(level) << "if (" << numberExpr(statement.left, symbols, actorPtrName)
                            << " == " << numberExpr(statement.right, symbols, actorPtrName) << ") {\n";
                    }
                    emitStatements(out, program, statement.body, symbols, level + 1, nextInputId, actorPtrName);
                    out << indent(level) << "}\n";
                    break;
                case hyh::fairy::Statement::Kind::Call:
                    if (statement.arguments.empty()) {
                        out << indent(level) << "/* call missing argument */\n";
                    } else {
                        out << indent(level) << "std::println(" << q(statement.arguments.front()) << ");\n";
                    }
                    break;
                case hyh::fairy::Statement::Kind::Speech:
                    out << indent(level) << "std::println(" << q(statement.speechText) << ");\n";
                    break;
                case hyh::fairy::Statement::Kind::Input: {
                    emitMirrorInput(out, symbols, statement, level);
                    break;
                }
                case hyh::fairy::Statement::Kind::NarrativeEvent:
                    out << indent(level) << "/* narrative event */\n";
                    break;
                }
            }
        }

    } // namespace

    std::string CppCodeGenerator::generate(const hyh::fairy::Program& program) const {
        EntityProperties properties;
        collectStatements(program, program.statements, properties);
        const auto hoistedInitializers = collectHoistedInitializers(program.statements);
        const auto symbols = makeSymbols(program, properties);

        std::ostringstream out;
        out << "#include <array>\n";
        out << "#include <iostream>\n";
        out << "#include <print>\n";
        out << "#include <string>\n";
        out << "#include <string_view>\n\n";
        out << "int main() {\n";
        out << "    using std::string_view;\n\n";
        for (const auto& type: symbols.types) {
            out << "    struct " << type.name << " {\n";
            for (const auto& propertyName: type.properties) {
                out << "        int " << symbols.propertyFields.at(propertyName)
                    << " = 0; // " << escapeCppString(propertyName) << "\n";
            }
            out << "        string_view name;\n";
            out << "    };\n";
        }
        out << '\n';

        std::set<std::string> entitiesInRoleGroups;
        std::set<std::string> declaredEntityRefs;
        for (const auto& roleName: sortedRoleNames(program)) {
            const auto group = program.roleGroups.find(roleName);
            const auto groupVar = symbols.roleGroupVars.find(roleName);
            if (group == program.roleGroups.end() || groupVar == symbols.roleGroupVars.end()
                || group->second.members.empty()) {
                continue;
            }

            const auto firstMember = group->second.members.front();
            out << "    std::array<" << symbols.entityTypes.at(firstMember) << ", " << group->second.members.size()
                << "> " << groupVar->second << "{{\n";
            for (const auto& member: group->second.members) {
                out << "        " << entityInitializer(symbols, member, hoistedInitializers.values) << ",\n";
                entitiesInRoleGroups.insert(member);
            }
            out << "    }};\n";
            out << "    (void)" << groupVar->second << ";\n";

            for (std::size_t i = 0; i < group->second.members.size(); ++i) {
                const auto& member = group->second.members[i];
                if (declaredEntityRefs.contains(member)) {
                    continue;
                }
                out << "    auto& " << symbols.entityVars.at(member) << " = " << groupVar->second << "["
                    << i << "];\n";
                out << "    (void)" << symbols.entityVars.at(member) << ";\n";
                declaredEntityRefs.insert(member);
            }
            out << '\n';
        }

        for (const auto& entityName: sortedEntityNames(program, properties)) {
            if (entitiesInRoleGroups.contains(entityName)) {
                continue;
            }
            out << "    " << symbols.entityTypes.at(entityName) << ' ' << symbols.entityVars.at(entityName)
                << entityInitializer(symbols, entityName, hoistedInitializers.values) << ";\n";
            out << "    (void)" << symbols.entityVars.at(entityName) << ";\n\n";
        }

        int nextInputId = 1;
        emitStatements(out, program, program.statements, symbols, 1, nextInputId, {}, &hoistedInitializers.statements);

        out << "    return 0;\n";
        out << "}\n";
        return out.str();
    }

} // namespace hyh::codegen
