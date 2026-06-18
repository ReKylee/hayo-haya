#include "driver.hpp"

#include "fairy/semantic_dump.hpp"
#include "hyh_grammar_parser.hpp"
#include "lexer.hpp"

#include <iostream>

namespace hyh {
    namespace {

        std::size_t lineIndent(const RawLine& line) {
            if (line.lexemes.empty()) {
                return 0;
            }
            return line.lexemes.front().column;
        }

        struct TreeFrame {
            std::size_t indent = 0;
            std::vector<RawNode>* nodes = nullptr;
        };

    } // namespace

    bool Driver::parse(const std::string& source) {
        rawLines_.clear();
        lattices_.clear();
        sentenceIr_.clear();
        program_ = fairy::Program{};
        syntaxErrors_.clear();

        Lexer lexer(source);
        GrammarParser parser(lexer, *this);
        const int result = parser.parse();
        if (result != 0 || !syntaxErrors_.empty()) {
            for (const auto& error: syntaxErrors_) {
                program_.diagnostics.push_back(fairy::Diagnostic{"SYNTAX", 0, error});
            }
            return false;
        }

        const auto tree = buildRawTree();
        auto semanticNodes = analyzeNodes(tree);
        program_ = fairy::SemanticAnalyzer{}.analyze(semanticNodes);
        return program_.diagnostics.empty();
    }

    void Driver::addRawLine(std::vector<Lexeme> lexemes, RawLineTerminator terminator) {
        if (lexemes.empty()) {
            return;
        }
        RawLine line;
        line.line = lexemes.front().line;
        line.indent = lineIndent(RawLine{.lexemes = lexemes});
        line.terminator = terminator;
        line.lexemes = std::move(lexemes);
        rawLines_.push_back(std::move(line));
    }

    void Driver::addSyntaxError(const std::string& message) {
        syntaxErrors_.push_back(message);
    }

    std::vector<RawNode> Driver::buildRawTree() const {
        std::vector<RawNode> roots;
        std::vector<TreeFrame> stack;
        stack.push_back(TreeFrame{0, &roots});

        for (const auto& line: rawLines_) {
            const auto indent = line.indent;
            while (stack.size() > 1 && indent <= stack.back().indent) {
                stack.pop_back();
            }

            stack.back().nodes->push_back(RawNode{line, {}});
            auto& inserted = stack.back().nodes->back();
            if (line.startsBlock()) {
                stack.push_back(TreeFrame{indent, &inserted.children});
            }
        }
        return roots;
    }

    std::vector<fairy::SemanticInputNode> Driver::analyzeNodes(const std::vector<RawNode>& nodes) {
        std::vector<fairy::SemanticInputNode> result;
        hebrew::Analyzer analyzer;
        hebrew::SentenceAnalyzer sentenceAnalyzer;

        for (const auto& node: nodes) {
            auto lattice = analyzer.analyze(node.line.lexemes);
            auto sentence = sentenceAnalyzer.analyze(lattice, node.line);
            lattices_.push_back(lattice);
            sentenceIr_.push_back(sentence);
            fairy::SemanticInputNode semanticNode;
            semanticNode.sentence = std::move(sentence);
            semanticNode.children = analyzeNodes(node.children);
            result.push_back(std::move(semanticNode));
        }
        return result;
    }

    void Driver::dumpLattice(std::ostream& out) const {
        for (const auto& lattice: lattices_) {
            hebrew::dumpLattice(out, lattice);
        }
    }

    void Driver::dumpHebrewIr(std::ostream& out) const {
        for (const auto& sentence: sentenceIr_) {
            hebrew::dumpSentenceIr(out, sentence);
        }
    }

    void Driver::dumpSemanticIr(std::ostream& out) const {
        fairy::dumpSemanticIr(out, program_);
    }

    void Driver::dumpIr(std::ostream& out) const {
        out << "== Hebrew sentence IR ==\n";
        dumpHebrewIr(out);
        out << "== Resolved semantic IR ==\n";
        dumpSemanticIr(out);
    }

} // namespace hyh
