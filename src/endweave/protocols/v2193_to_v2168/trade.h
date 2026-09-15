#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/inventory.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::UpdateTradePacket_<2193>, bp::UpdateTradePacket_<2168>> {
    static void transform(Context<bp::UpdateTradePacket_<2168>> &ctx, bp::UpdateTradePacket_<2193> &&from);
};

template <>
struct Transformer<bp::UpdateEquipPacket_<2193>, bp::UpdateEquipPacket_<2168>> {
    static void transform(Context<bp::UpdateEquipPacket_<2168>> &ctx, bp::UpdateEquipPacket_<2193> &&from);
};

} // namespace endweave
