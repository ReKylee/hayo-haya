#include "codegen/cpp_codegen.hpp"
#include "driver.hpp"

#include <args.hxx>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

    std::string readFile(const std::filesystem::path& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            throw std::runtime_error("could not open input file: " + path.string());
        }
        std::ostringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }

    void writeFile(const std::filesystem::path& path, const std::string& text) {
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
        std::ofstream out(path, std::ios::binary);
        if (!out) {
            throw std::runtime_error("could not open output file: " + path.string());
        }
        out << text;
    }

    std::string defaultGeneratedCppPath(const std::string& inputPath) {
        auto path = std::filesystem::path(inputPath).filename();
        path.replace_extension(".cpp");
        return (std::filesystem::path("generated") / path).string();
    }

    std::string defaultExePath(const std::string& inputPath) {
        auto path = std::filesystem::path(inputPath).filename();
#ifdef _WIN32
        path.replace_extension(".exe");
#else
        path.replace_extension("");
#endif
        return (std::filesystem::path("bin") / path).string();
    }

    void ensureParentDirectory(const std::filesystem::path& path) {
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
    }

    std::string quoteCommandArg(const std::string& arg) {
#ifdef _WIN32
        std::string quoted = "\"";
        for (const char c: arg) {
            if (c == '"') {
                quoted += "\\\"";
            } else {
                quoted += c;
            }
        }
        quoted += '"';
        return quoted;
#else
        std::string quoted = "'";
        for (const char c: arg) {
            if (c == '\'') {
                quoted += "'\\''";
            } else {
                quoted += c;
            }
        }
        quoted += '\'';
        return quoted;
#endif
    }

    std::string selectCxxCompiler(const hyh::CompileOptions& options) {
        if (!options.cxxCompiler.empty()) {
            return options.cxxCompiler;
        }
        if (const char* env = std::getenv("HYH_CXX")) {
            return env;
        }
        if (const char* env = std::getenv("CXX")) {
            return env;
        }
        return "clang++";
    }

    std::string defaultLinkFlags() {
#ifdef _WIN32
        return " -lstdc++exp";
#else
        return "";
#endif
    }

    hyh::CompileOptions parseOptions(int argc, char** argv) {
        args::ArgumentParser parser("hyh: a Hebrew fairy-tale compiler.",
                                    "Default output: generated/<story>.cpp and bin/<story>[.exe].");

        args::HelpFlag help(parser, "help", "Show this help message", {"h", "help"});
        args::ValueFlag<std::string> outputCpp(parser, "path", "Generated C++ output path", {"o", "output"});
        args::ValueFlag<std::string> outputExe(parser, "path", "Executable output path", {"exe"});
        args::ValueFlag<std::string> cxxCompiler(parser, "compiler", "C++ compiler for generated code", {"cxx"});
        args::Flag compile(parser, "compile", "Compile generated C++ to an executable; already the default",
                           {"compile"});
        args::Flag noCompile(parser, "no-compile", "Only emit generated C++; do not compile it", {"no-compile"});
        args::Flag dumpLattice(parser, "dump-lattice", "Dump Hebrew morphological lattice", {"dump-lattice"});
        args::Flag dumpIr(parser, "dump-ir", "Dump both Hebrew sentence IR and resolved semantic IR", {"dump-ir"});
        args::Flag dumpHebrewIr(parser, "dump-hebrew-ir", "Dump selected Hebrew sentence IR only", {"dump-hebrew-ir"});
        args::Flag dumpSemanticIr(parser, "dump-semantic-ir", "Dump resolved semantic story IR only",
                                  {"dump-semantic-ir"});
        args::Positional<std::string> input(parser, "input.hyh", "Input story file");

        try {
            parser.ParseCLI(argc, argv);
        } catch (const args::Help&) {
            std::cout << parser;
            std::exit(0);
        } catch (const args::ValidationError& error) {
            std::cerr << error.what() << '\n' << parser;
            std::exit(2);
        } catch (const args::ParseError& error) {
            std::cerr << error.what() << '\n' << parser;
            std::exit(2);
        }

        if (compile && noCompile) {
            std::cerr << "cannot use both --compile and --no-compile\n" << parser;
            std::exit(2);
        }

        hyh::CompileOptions options;
        options.compileGeneratedCpp = !static_cast<bool>(noCompile);
        options.dumpLattice = static_cast<bool>(dumpLattice);
        options.dumpHebrewIr = static_cast<bool>(dumpIr) || static_cast<bool>(dumpHebrewIr);
        options.dumpSemanticIr = static_cast<bool>(dumpIr) || static_cast<bool>(dumpSemanticIr);
        if (input) {
            options.inputPath = args::get(input);
        }
        if (outputCpp) {
            options.outputCppPath = args::get(outputCpp);
        }
        if (outputExe) {
            options.outputExePath = args::get(outputExe);
        }
        if (cxxCompiler) {
            options.cxxCompiler = args::get(cxxCompiler);
        }
        if (options.inputPath.empty()) {
            std::cerr << parser;
            std::exit(2);
        }
        if (options.outputCppPath.empty()) {
            options.outputCppPath = defaultGeneratedCppPath(options.inputPath);
        }
        if (options.compileGeneratedCpp && options.outputExePath.empty()) {
            options.outputExePath = defaultExePath(options.inputPath);
        }
        return options;
    }

} // namespace

int main(int argc, char** argv) {
    try {
        const auto options = parseOptions(argc, argv);
        const auto source = readFile(options.inputPath);

        hyh::Driver driver;
        const bool ok = driver.parse(source);

        if (options.dumpLattice) {
            driver.dumpLattice(std::cout);
        }
        if (options.dumpHebrewIr && options.dumpSemanticIr) {
            driver.dumpIr(std::cout);
        } else {
            if (options.dumpHebrewIr) {
                driver.dumpHebrewIr(std::cout);
            }
            if (options.dumpSemanticIr) {
                driver.dumpSemanticIr(std::cout);
            }
        }
        for (const auto& diagnostic: driver.program().diagnostics) {
            std::cerr << diagnostic.code << " line " << diagnostic.line << ": " << diagnostic.message << '\n';
        }
        if (!ok) {
            return 1;
        }

        const auto generated = hyh::codegen::CppCodeGenerator{}.generate(driver.program());
        writeFile(options.outputCppPath, generated);
        std::cout << "wrote generated C++: " << options.outputCppPath << '\n';

        if (!options.compileGeneratedCpp) {
            return 0;
        }

        ensureParentDirectory(options.outputExePath);
        const auto selectedCompiler = selectCxxCompiler(options);
        const std::string command = selectedCompiler + " -std=c++2b " + quoteCommandArg(options.outputCppPath) + " -o "
                                  + quoteCommandArg(options.outputExePath) + defaultLinkFlags();
        std::cout << "running C++ compiler: " << command << '\n';
        const int result = std::system(command.c_str());
        if (result != 0) {
            std::cerr << "failed to compile generated C++ with command: " << command << '\n';
            return result;
        }
        std::cout << "wrote executable: " << options.outputExePath << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "hyh: " << error.what() << '\n';
        return 1;
    }
}
