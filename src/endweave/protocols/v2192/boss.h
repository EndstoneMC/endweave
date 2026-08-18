#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/boss.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::BossEventPacket_<2192>, bp::BossEventPacket_<2168>> {
    static void transform(Context<bp::BossEventPacket_<2168>> &ctx, bp::BossEventPacket_<2192> &&from);
};

} // namespace endweave
