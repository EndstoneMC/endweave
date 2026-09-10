#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/input.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlayerAuthInputPacket_<2192>::InputData, bp::PlayerAuthInputPacket_<2168>::InputData> {
    static void transform(Context<bp::PlayerAuthInputPacket_<2168>::InputData> &ctx,
                          bp::PlayerAuthInputPacket_<2192>::InputData &&from);
};

template <>
struct Transformer<bp::PackedItemUseLegacyInventoryTransaction_<2192>,
                   bp::PackedItemUseLegacyInventoryTransaction_<2168>> {
    static void transform(Context<bp::PackedItemUseLegacyInventoryTransaction_<2168>> &ctx,
                          bp::PackedItemUseLegacyInventoryTransaction_<2192> &&from);
};

template <>
struct Transformer<bp::PlayerAuthInputPacket_<2192>, bp::PlayerAuthInputPacket_<2168>> {
    static void transform(Context<bp::PlayerAuthInputPacket_<2168>> &ctx, bp::PlayerAuthInputPacket_<2192> &&from);
};

} // namespace endweave
