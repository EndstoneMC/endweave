#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/inventory.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::SerializedNetworkItemStackDescriptor_<2168>, bp::SerializedNetworkItemStackDescriptor_<1001>> {
    static void transform(Context<bp::SerializedNetworkItemStackDescriptor_<1001>> &ctx,
                          bp::SerializedNetworkItemStackDescriptor_<2168> &&from);
};

template <>
struct Transformer<bp::InventoryAction_<2168>, bp::InventoryAction_<1001>> {
    static void transform(Context<bp::InventoryAction_<1001>> &ctx, bp::InventoryAction_<2168> &&from);
};

template <>
struct Transformer<bp::ItemUseOnActorInventoryTransaction_<2168>, bp::ItemUseOnActorInventoryTransaction_<1001>> {
    static void transform(Context<bp::ItemUseOnActorInventoryTransaction_<1001>> &ctx,
                          bp::ItemUseOnActorInventoryTransaction_<2168> &&from);
};

template <>
struct Transformer<bp::ItemReleaseInventoryTransaction_<2168>, bp::ItemReleaseInventoryTransaction_<1001>> {
    static void transform(Context<bp::ItemReleaseInventoryTransaction_<1001>> &ctx,
                          bp::ItemReleaseInventoryTransaction_<2168> &&from);
};

template <>
struct Transformer<bp::TransactionData_<2168>, bp::TransactionData_<1001>> {
    static void transform(Context<bp::TransactionData_<1001>> &ctx, bp::TransactionData_<2168> &&from);
};

template <>
struct Transformer<bp::ItemUseInventoryTransaction_<2168>, bp::ItemUseInventoryTransaction_<2192>> {
    static void transform(Context<bp::ItemUseInventoryTransaction_<2192>> &ctx,
                          bp::ItemUseInventoryTransaction_<2168> &&from);
};

template <>
struct Transformer<bp::ItemUseOnActorInventoryTransaction_<2168>, bp::ItemUseOnActorInventoryTransaction_<2192>> {
    static void transform(Context<bp::ItemUseOnActorInventoryTransaction_<2192>> &ctx,
                          bp::ItemUseOnActorInventoryTransaction_<2168> &&from);
};

template <>
struct Transformer<bp::ItemReleaseInventoryTransaction_<2168>, bp::ItemReleaseInventoryTransaction_<2192>> {
    static void transform(Context<bp::ItemReleaseInventoryTransaction_<2192>> &ctx,
                          bp::ItemReleaseInventoryTransaction_<2168> &&from);
};

template <>
struct Transformer<bp::TransactionData_<2168>, bp::TransactionData_<2192>> {
    static void transform(Context<bp::TransactionData_<2192>> &ctx, bp::TransactionData_<2168> &&from);
};

} // namespace endweave
