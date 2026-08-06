#pragma once

#include "endweave/protocol/transform.h"
#include "endweave/protocols/v2168/item_stack.h"

#include <protocol/input.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlayerBlockActionData_<2168>, bp::PlayerBlockActionData_<1001>> {
    static bp::PlayerBlockActionData_<1001> transform(bp::PlayerBlockActionData_<2168> &&from);
};

template <>
struct Transformer<bp::PlayerActionPacket_<2168>, bp::PlayerActionPacket_<1001>> {
    static bp::PlayerActionPacket_<1001> transform(bp::PlayerActionPacket_<2168> &&from);
};

template <>
struct Transformer<bp::PackedItemUseLegacyInventoryTransaction_<2168>,
                   bp::PackedItemUseLegacyInventoryTransaction_<1001>> {
    static bp::PackedItemUseLegacyInventoryTransaction_<1001> transform(
        bp::PackedItemUseLegacyInventoryTransaction_<2168> &&from);
};

template <>
struct Transformer<bp::PlayerAuthInputPacket_<2168>, bp::PlayerAuthInputPacket_<1001>> {
    static bp::PlayerAuthInputPacket_<1001> transform(bp::PlayerAuthInputPacket_<2168> &&from);
};

} // namespace endweave
