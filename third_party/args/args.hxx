#pragma once

#include <exception>
#include <ostream>
#include <string>
#include <vector>

namespace args {

    struct Help : std::exception {};

    class ParseError : public std::exception {
    public:
        explicit ParseError(std::string message) : message_(std::move(message)) {}
        const char* what() const noexcept override {
            return message_.c_str();
        }

    private:
        std::string message_;
    };

    class ValidationError : public ParseError {
    public:
        using ParseError::ParseError;
    };

    class ArgumentParser;

    class FlagBase {
    public:
        FlagBase(ArgumentParser& parser, std::string name, std::string help, std::vector<std::string> flags);
        virtual ~FlagBase() = default;
        [[nodiscard]] const std::vector<std::string>& flags() const {
            return flags_;
        }
        [[nodiscard]] const std::string& help() const {
            return help_;
        }
        virtual void parseValue(const std::vector<std::string>& argv, int& index) = 0;

    protected:
        std::string name_;
        std::string help_;
        std::vector<std::string> flags_;
    };

    class ArgumentParser {
    public:
        ArgumentParser(std::string description, std::string epilog = {}) :
                description_(std::move(description)),
                epilog_(std::move(epilog)) {}
        void addFlag(FlagBase& flag) {
            flags_.push_back(&flag);
        }
        void setPositionalTarget(std::string* target) {
            positional_ = target;
        }

        void ParseCLI(int argc, char** argv) {
            std::vector<std::string> args;
            for (int i = 0; i < argc; ++i) {
                args.emplace_back(argv[i]);
            }
            for (int i = 1; i < static_cast<int>(args.size()); ++i) {
                const auto& arg = args[static_cast<std::size_t>(i)];
                if (arg == "-h" || arg == "--help") {
                    throw Help{};
                }
                if (!arg.empty() && arg[0] == '-') {
                    FlagBase* found = nullptr;
                    for (auto* flag: flags_) {
                        for (const auto& name: flag->flags()) {
                            if (arg == name) {
                                found = flag;
                                break;
                            }
                        }
                        if (found) {
                            break;
                        }
                    }
                    if (!found) {
                        throw ParseError("unknown option: " + arg);
                    }
                    found->parseValue(args, i);
                } else {
                    if (!positional_) {
                        throw ParseError("unexpected positional argument: " + arg);
                    }
                    if (!positional_->empty()) {
                        throw ParseError("unexpected extra input: " + arg);
                    }
                    *positional_ = arg;
                }
            }
        }

        friend std::ostream& operator<<(std::ostream& out, const ArgumentParser& parser) {
            out << parser.description_ << "\n\n";
            out << "Options:\n";
            for (const auto* flag: parser.flags_) {
                out << "  ";
                for (std::size_t i = 0; i < flag->flags().size(); ++i) {
                    if (i > 0) {
                        out << ", ";
                    }
                    out << flag->flags()[i];
                }
                out << "\n      " << flag->help() << "\n";
            }
            if (!parser.epilog_.empty()) {
                out << "\n" << parser.epilog_ << "\n";
            }
            return out;
        }

    private:
        std::string description_;
        std::string epilog_;
        std::vector<FlagBase*> flags_;
        std::string* positional_ = nullptr;
    };

    inline std::vector<std::string> normalizeFlags(std::initializer_list<const char*> flags) {
        std::vector<std::string> result;
        for (const char* flag: flags) {
            std::string value = flag;
            if (value.size() == 1) {
                result.push_back("-" + value);
            } else {
                result.push_back("--" + value);
            }
        }
        return result;
    }

    inline FlagBase::FlagBase(ArgumentParser& parser, std::string name, std::string help,
                              std::vector<std::string> flags) :
            name_(std::move(name)),
            help_(std::move(help)),
            flags_(std::move(flags)) {
        parser.addFlag(*this);
    }

    class HelpFlag : public FlagBase {
    public:
        HelpFlag(ArgumentParser& parser, const std::string& name, const std::string& help,
                 std::initializer_list<const char*> flags) :
                FlagBase(parser, name, help, normalizeFlags(flags)) {}
        void parseValue(const std::vector<std::string>&, int&) override {
            throw Help{};
        }
    };

    class Flag : public FlagBase {
    public:
        Flag(ArgumentParser& parser, const std::string& name, const std::string& help,
             std::initializer_list<const char*> flags) :
                FlagBase(parser, name, help, normalizeFlags(flags)) {}
        void parseValue(const std::vector<std::string>&, int&) override {
            value_ = true;
        }
        explicit operator bool() const {
            return value_;
        }

    private:
        bool value_ = false;
    };

    template<typename T>
    class ValueFlag : public FlagBase {
    public:
        ValueFlag(ArgumentParser& parser, const std::string&, const std::string& help,
                  std::initializer_list<const char*> flags) :
                FlagBase(parser, {}, help, normalizeFlags(flags)) {}
        void parseValue(const std::vector<std::string>& argv, int& index) override {
            if (index + 1 >= static_cast<int>(argv.size())) {
                throw ParseError("missing value for " + argv[static_cast<std::size_t>(index)]);
            }
            value_ = argv[static_cast<std::size_t>(++index)];
            hasValue_ = true;
        }
        explicit operator bool() const {
            return hasValue_;
        }
        [[nodiscard]] const T& value() const {
            return value_;
        }

    private:
        T value_{};
        bool hasValue_ = false;
    };

    template<typename T>
    class Positional {
    public:
        Positional(ArgumentParser& parser, const std::string&, const std::string&) {
            parser.setPositionalTarget(&value_);
        }
        explicit operator bool() const {
            return !value_.empty();
        }
        [[nodiscard]] const T& value() const {
            return value_;
        }

    private:
        T value_{};
    };

    template<typename T>
    const T& get(const ValueFlag<T>& flag) {
        return flag.value();
    }

    template<typename T>
    const T& get(const Positional<T>& positional) {
        return positional.value();
    }

} // namespace args
