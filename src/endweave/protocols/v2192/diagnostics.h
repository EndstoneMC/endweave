#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/diagnostics.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::MemoryCategoryCounter_<2192>, bp::MemoryCategoryCounter_<2168>> {
    static void transform(Context<bp::MemoryCategoryCounter_<2168>> &ctx, bp::MemoryCategoryCounter_<2192> &&from);
};

} // namespace endweave
