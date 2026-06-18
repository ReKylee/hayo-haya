#pragma once

#include "fairy/story_ast.hpp"

namespace hyh::fairy {

    class MirrorTypeResolver {
    public:
        void resolve(Program& program) const;
    };

} // namespace hyh::fairy
