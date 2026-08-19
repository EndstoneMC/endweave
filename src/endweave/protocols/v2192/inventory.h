#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/inventory.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::ItemUseOnActorInventoryTransaction_<2192>, bp::ItemUseOnActorInventoryTransaction_<2168>> {
    static void transform(Context<bp::ItemUseOnActorInventoryTransaction_<2168>> &ctx,
                          bp::ItemUseOnActorInventoryTransaction_<2192> &&from);
};

template <>
struct Transformer<bp::ItemReleaseInventoryTransaction_<2192>, bp::ItemReleaseInventoryTransaction_<2168>> {
    static void transform(Context<bp::ItemReleaseInventoryTransaction_<2168>> &ctx,
                          bp::ItemReleaseInventoryTransaction_<2192> &&from);
};

template <>
struct Transformer<bp::TransactionData_<2192>, bp::TransactionData_<2168>> {
    static void transform(Context<bp::TransactionData_<2168>> &ctx, bp::TransactionData_<2192> &&from);
};

} // namespace endweave
