#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/block.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::AnvilDamagePacket_<1001>> {
    static bp::AnvilDamagePacket_<2168> upgrade(bp::AnvilDamagePacket_<1001> &&from);
};

} // namespace endweave
