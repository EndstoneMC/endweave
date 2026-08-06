#pragma once

#include "endweave/protocol/transform.h"
#include "endweave/protocols/v1001/item_stack.h"

#include <protocol/input.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlayerBlockActionData_<1001>, bp::PlayerBlockActionData_<2168>> {
    static bp::PlayerBlockActionData_<2168> transform(bp::PlayerBlockActionData_<1001> &&from);
};

template <>
struct Transformer<bp::PlayerActionPacket_<1001>, bp::PlayerActionPacket_<2168>> {
    static bp::PlayerActionPacket_<2168> transform(bp::PlayerActionPacket_<1001> &&from);
};

template <>
struct Transformer<bp::PackedItemUseLegacyInventoryTransaction_<1001>,
                   bp::PackedItemUseLegacyInventoryTransaction_<2168>> {
    static bp::PackedItemUseLegacyInventoryTransaction_<2168> transform(
        bp::PackedItemUseLegacyInventoryTransaction_<1001> &&from);
};

template <>
struct Transformer<bp::PlayerAuthInputPacket_<1001>, bp::PlayerAuthInputPacket_<2168>> {
    static bp::PlayerAuthInputPacket_<2168> transform(bp::PlayerAuthInputPacket_<1001> &&from);
};

} // namespace endweave
