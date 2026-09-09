#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/inventory.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::UpdateTradePacket_<2168>, bp::UpdateTradePacket_<2192>> {
    static void transform(Context<bp::UpdateTradePacket_<2192>> &ctx, bp::UpdateTradePacket_<2168> &&from);
};

template <>
struct Transformer<bp::UpdateEquipPacket_<2168>, bp::UpdateEquipPacket_<2192>> {
    static void transform(Context<bp::UpdateEquipPacket_<2192>> &ctx, bp::UpdateEquipPacket_<2168> &&from);
};
} // namespace endweave
