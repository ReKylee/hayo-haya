#pragma once

#include <iosfwd>
#include <string>
#include <vector>

#include "fairy/semantic_analyzer.hpp"
#include "fairy/story_ast.hpp"
#include "hebrew/morphology.hpp"
#include "hebrew/sentence_ir.hpp"
#include "raw/raw_syntax.hpp"

namespace hyh {

    struct CompileOptions {
        bool compileGeneratedCpp = true;
        bool dumpLattice = false;
        bool dumpHebrewIr = false;
        bool dumpSemanticIr = false;
        std::string inputPath;
        std::string outputCppPath;
        std::string outputExePath;
        std::string cxxCompiler;
    };

    class Driver {
    public:
        bool parse(const std::string& source);

        void addRawLine(std::vector<Lexeme> lexemes, RawLineTerminator terminator);
        void addSyntaxError(const std::string& message);

        [[nodiscard]] const fairy::Program& program() const {
            return program_;
        }

        void dumpLattice(std::ostream& out) const;
        void dumpHebrewIr(std::ostream& out) const;
        void dumpSemanticIr(std::ostream& out) const;
        void dumpIr(std::ostream& out) const;

    private:
        std::vector<RawNode> buildRawTree() const;
        std::vector<fairy::SemanticInputNode> analyzeNodes(const std::vector<RawNode>& nodes);

        std::vector<RawLine> rawLines_;
        std::vector<hebrew::MorphLattice> lattices_;
        std::vector<hebrew::HebrewSentence> sentenceIr_;
        fairy::Program program_;
        std::vector<std::string> syntaxErrors_;
    };

} // namespace hyh
