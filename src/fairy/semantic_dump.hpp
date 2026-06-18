#pragma once

#include <iosfwd>

#include "fairy/story_ast.hpp"

namespace hyh::fairy {

    void dumpSemanticIr(std::ostream& out, const Program& program);

} // namespace hyh::fairy
