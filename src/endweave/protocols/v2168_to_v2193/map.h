#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/map.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::MapDecoration_<2168>, bp::MapDecoration_<2193>> {
    static void transform(Context<bp::MapDecoration_<2193>> &ctx, bp::MapDecoration_<2168> &&from);
};
} // namespace endweave
