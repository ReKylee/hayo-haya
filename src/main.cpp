#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "codegen.hpp"
#include "driver.hpp"

#ifndef HYH_DEFAULT_CXX_COMPILER
#define HYH_DEFAULT_CXX_COMPILER "c++"
#endif

namespace {

struct Options {
    std::string inputPath;
    std::string outputPath = "out/story.cpp";
    std::string executablePath;
    bool compile = true;
};

static std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file: " + path);
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

static void printUsage() {
    std::cerr << "usage: hyh <file.hyh> [-o output.cpp] [--exe output_executable] [--compile|--no-compile]\n";
}

static Options parseOptions(int argc, char** argv) {
    if (argc < 2) {
        printUsage();
        throw std::runtime_error("missing input file");
    }

    Options options;
    options.inputPath = argv[1];

    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-o") {
            if (++i >= argc) {
                printUsage();
                throw std::runtime_error("missing value after -o");
            }
            options.outputPath = argv[i];
        } else if (arg == "--exe") {
            if (++i >= argc) {
                printUsage();
                throw std::runtime_error("missing value after --exe");
            }
            options.executablePath = argv[i];
        } else if (arg == "--compile") {
            options.compile = true;
        } else if (arg == "--no-compile") {
            options.compile = false;
        } else {
            printUsage();
            throw std::runtime_error("unknown option: " + arg);
        }
    }

    return options;
}

static std::string defaultExecutablePath(const std::string& outputPath) {
    std::filesystem::path executable = outputPath;
    executable.replace_extension();
#ifdef _WIN32
    executable += ".exe";
#endif
    return executable.string();
}

static std::string commandQuote(const std::string& value) {
    std::string quoted = "\"";
    for (char c : value) {
        if (c == '"') {
            quoted += '\\';
        }
        quoted += c;
    }
    quoted += '"';
    return quoted;
}

static bool looksLikeMsvc(const std::string& compiler) {
    const std::filesystem::path path = compiler;
    const std::string filename = path.filename().string();
    return filename == "cl" || filename == "cl.exe";
}

static std::string backendCompiler() {
    if (const char* fromEnv = std::getenv("HYH_CXX")) {
        return fromEnv;
    }
    if (const char* fromEnv = std::getenv("CXX")) {
        return fromEnv;
    }
    return HYH_DEFAULT_CXX_COMPILER;
}

static void compileGeneratedCpp(const std::string& cppPath, const std::string& executablePath) {
    const std::string compiler = backendCompiler();
    std::string command;

    if (looksLikeMsvc(compiler)) {
        command = commandQuote(compiler)
            + " /nologo /std:c++latest /EHsc "
            + commandQuote(cppPath)
            + " /Fe:"
            + commandQuote(executablePath);
    } else {
        command = commandQuote(compiler)
            + " -std=c++23 "
            + commandQuote(cppPath)
            + " -o "
            + commandQuote(executablePath);
    }

    std::cout << "compiling: " << executablePath << "\n";
    const int result = std::system(command.c_str());
    if (result != 0) {
        throw std::runtime_error("C++ compilation failed");
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parseOptions(argc, argv);
        const std::string executablePath = options.executablePath.empty()
            ? defaultExecutablePath(options.outputPath)
            : options.executablePath;

        std::string source = readFile(options.inputPath);
        hyh::Driver driver(std::move(source));

        const int result = driver.parse();
        if (result != 0 || driver.hadError()) {
            std::cerr << "parse failed\n";
            return result == 0 ? 1 : result;
        }

        const std::filesystem::path outputFile = options.outputPath;
        if (outputFile.has_parent_path()) {
            std::filesystem::create_directories(outputFile.parent_path());
        }

        {
            std::ofstream out(options.outputPath, std::ios::binary);
            if (!out) {
                throw std::runtime_error("Could not open output file: " + options.outputPath);
            }

            hyh::CodeGenerator generator;
            generator.generate(driver.program(), out);
        }

        std::cout << "parsed successfully\n";
        std::cout << "generated: " << options.outputPath << "\n";

        if (options.compile) {
            const std::filesystem::path executableFile = executablePath;
            if (executableFile.has_parent_path()) {
                std::filesystem::create_directories(executableFile.parent_path());
            }
            compileGeneratedCpp(options.outputPath, executablePath);
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
