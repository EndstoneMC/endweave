#pragma once

#include "endweave/protocol/transform.h"
#include "endweave/protocols/v2168/item_stack.h"

#include <protocol/input.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlayerBlockActionData_<2168>, bp::PlayerBlockActionData_<1001>> {
    static void transform(Context<bp::PlayerBlockActionData_<1001>> &ctx, bp::PlayerBlockActionData_<2168> &&from);
};

template <>
struct Transformer<bp::PlayerActionPacket_<2168>, bp::PlayerActionPacket_<1001>> {
    static void transform(Context<bp::PlayerActionPacket_<1001>> &ctx, bp::PlayerActionPacket_<2168> &&from);
};

template <>
struct Transformer<bp::PackedItemUseLegacyInventoryTransaction_<2168>,
                   bp::PackedItemUseLegacyInventoryTransaction_<1001>> {
    static void transform(Context<bp::PackedItemUseLegacyInventoryTransaction_<1001>> &ctx,
                          bp::PackedItemUseLegacyInventoryTransaction_<2168> &&from);
};

template <>
struct Transformer<bp::PlayerAuthInputPacket_<2168>, bp::PlayerAuthInputPacket_<1001>> {
    static void transform(Context<bp::PlayerAuthInputPacket_<1001>> &ctx, bp::PlayerAuthInputPacket_<2168> &&from);
};

} // namespace endweave
