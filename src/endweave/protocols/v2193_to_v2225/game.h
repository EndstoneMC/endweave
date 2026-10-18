#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/game.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::LevelSettings_<2193>, bp::LevelSettings_<2225>> {
    static void transform(Context<bp::LevelSettings_<2225>> &ctx, bp::LevelSettings_<2193> &&from);
};
} // namespace endweave
