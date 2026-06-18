#pragma once

#include <string>

#include "fairy/story_ast.hpp"

namespace hyh::codegen {

    class CppCodeGenerator {
    public:
        [[nodiscard]] std::string generate(const hyh::fairy::Program& program) const;
    };

} // namespace hyh::codegen
