#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/inventory.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::NetworkItemStackDescriptor, bp::SerializedNetworkItemStackDescriptor_<2168>> {
    static void transform(Context<bp::SerializedNetworkItemStackDescriptor_<2168>> &ctx,
                          bp::NetworkItemStackDescriptor &&from);
};

template <>
struct Transformer<bp::SerializedNetworkItemStackDescriptor_<1001>, bp::SerializedNetworkItemStackDescriptor_<2168>> {
    static void transform(Context<bp::SerializedNetworkItemStackDescriptor_<2168>> &ctx,
                          bp::SerializedNetworkItemStackDescriptor_<1001> &&from);
};

template <>
struct Transformer<bp::InventoryAction_<1001>, bp::InventoryAction_<2168>> {
    static void transform(Context<bp::InventoryAction_<2168>> &ctx, bp::InventoryAction_<1001> &&from);
};

template <>
struct Transformer<bp::legacy::ItemUseInventoryTransaction_<1001>, bp::ItemUseInventoryTransaction_<1001>> {
    static void transform(Context<bp::ItemUseInventoryTransaction_<1001>> &ctx,
                          bp::legacy::ItemUseInventoryTransaction_<1001> &&from);
};

template <>
struct Transformer<bp::ItemUseInventoryTransaction_<1001>, bp::legacy::ItemUseInventoryTransaction_<1001>> {
    static void transform(Context<bp::legacy::ItemUseInventoryTransaction_<1001>> &ctx,
                          bp::ItemUseInventoryTransaction_<1001> &&from);
};

template <>
struct Transformer<bp::ItemUseOnActorInventoryTransaction_<1001>, bp::ItemUseOnActorInventoryTransaction_<2168>> {
    static void transform(Context<bp::ItemUseOnActorInventoryTransaction_<2168>> &ctx,
                          bp::ItemUseOnActorInventoryTransaction_<1001> &&from);
};

template <>
struct Transformer<bp::ItemReleaseInventoryTransaction_<1001>, bp::ItemReleaseInventoryTransaction_<2168>> {
    static void transform(Context<bp::ItemReleaseInventoryTransaction_<2168>> &ctx,
                          bp::ItemReleaseInventoryTransaction_<1001> &&from);
};

template <>
struct Transformer<bp::TransactionData_<1001>, bp::TransactionData_<2168>> {
    static void transform(Context<bp::TransactionData_<2168>> &ctx, bp::TransactionData_<1001> &&from);
};

} // namespace endweave
