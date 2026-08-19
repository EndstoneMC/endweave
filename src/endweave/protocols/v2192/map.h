#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/map.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::MapDecoration_<2192>, bp::MapDecoration_<2168>> {
    static void transform(Context<bp::MapDecoration_<2168>> &ctx, bp::MapDecoration_<2192> &&from);
};

} // namespace endweave
