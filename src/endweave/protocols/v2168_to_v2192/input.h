#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/actor.h>
#include <bedrock/protocol/common.h>
#include <bedrock/protocol/input.h>
#include <bedrock/protocol/transaction.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::PlayerAuthInputPacket_<2168>::InputData, bp::PlayerAuthInputPacket_<2192>::InputData> {
    static void transform(Context<bp::PlayerAuthInputPacket_<2192>::InputData> &ctx,
                          bp::PlayerAuthInputPacket_<2168>::InputData &&from);
};

template <>
struct Transformer<bp::PackedItemUseLegacyInventoryTransaction_<2168>,
                   bp::PackedItemUseLegacyInventoryTransaction_<2192>> {
    static void transform(Context<bp::PackedItemUseLegacyInventoryTransaction_<2192>> &ctx,
                          bp::PackedItemUseLegacyInventoryTransaction_<2168> &&from);
};

template <>
struct Transformer<bp::PlayerAuthInputPacket_<2168>, bp::PlayerAuthInputPacket_<2192>> {
    static void transform(Context<bp::PlayerAuthInputPacket_<2192>> &ctx, bp::PlayerAuthInputPacket_<2168> &&from);
};
} // namespace endweave
