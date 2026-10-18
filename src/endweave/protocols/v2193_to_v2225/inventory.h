#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/inventory.h>
#include <bedrock/protocol/transaction.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::ItemUseOnActorInventoryTransaction_<2193>, bp::ItemUseOnActorInventoryTransaction_<2225>> {
    static void transform(Context<bp::ItemUseOnActorInventoryTransaction_<2225>> &ctx,
                          bp::ItemUseOnActorInventoryTransaction_<2193> &&from);
};

template <>
struct Transformer<bp::ItemReleaseInventoryTransaction_<2193>, bp::ItemReleaseInventoryTransaction_<2225>> {
    static void transform(Context<bp::ItemReleaseInventoryTransaction_<2225>> &ctx,
                          bp::ItemReleaseInventoryTransaction_<2193> &&from);
};
} // namespace endweave
