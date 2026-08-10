#pragma once

#include "endweave/protocol/transform.h"
#include "endweave/protocols/v1001/item_stack.h"

#include <protocol/input.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlayerBlockActionData_<1001>, bp::PlayerBlockActionData_<2168>> {
    static void transform(Context<bp::PlayerBlockActionData_<2168>> &ctx, bp::PlayerBlockActionData_<1001> &&from);
};

// ENDWEAVE: 2168 only appends INTERNAL_UPDATE, so every action 1001 can name keeps its value. The
// other direction has to rewrite that one and stays a Transformer.
template <>
struct WireCompatible<bp::PlayerActionPacket_<1001>, bp::PlayerActionPacket_<2168>> : std::true_type {};

template <>
struct Transformer<bp::PackedItemUseLegacyInventoryTransaction_<1001>,
                   bp::PackedItemUseLegacyInventoryTransaction_<2168>> {
    static void transform(Context<bp::PackedItemUseLegacyInventoryTransaction_<2168>> &ctx,
                          bp::PackedItemUseLegacyInventoryTransaction_<1001> &&from);
};

template <>
struct Transformer<bp::PlayerAuthInputPacket_<1001>, bp::PlayerAuthInputPacket_<2168>> {
    static void transform(Context<bp::PlayerAuthInputPacket_<2168>> &ctx, bp::PlayerAuthInputPacket_<1001> &&from);
};

} // namespace endweave
