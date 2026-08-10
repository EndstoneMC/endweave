#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/block.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::AnvilDamagePacket_<2168>, bp::AnvilDamagePacket_<1001>> {
    static void transform(Context<bp::AnvilDamagePacket_<1001>> &ctx, bp::AnvilDamagePacket_<2168> &&from);
};

} // namespace endweave
