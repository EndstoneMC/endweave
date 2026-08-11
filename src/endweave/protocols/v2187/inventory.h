#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/inventory.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::InventorySource_<2187>, bp::InventorySource_<2168>> {
    static void transform(Context<bp::InventorySource_<2168>> &ctx, bp::InventorySource_<2187> &&from);
};

template <>
struct Transformer<bp::InventoryAction_<2187>, bp::InventoryAction_<2168>> {
    static void transform(Context<bp::InventoryAction_<2168>> &ctx, bp::InventoryAction_<2187> &&from);
};

template <>
struct Transformer<bp::InventoryTransaction_<2187>, bp::InventoryTransaction_<2168>> {
    static void transform(Context<bp::InventoryTransaction_<2168>> &ctx, bp::InventoryTransaction_<2187> &&from);
};

template <>
struct Transformer<bp::NormalTransactionData_<2187>, bp::NormalTransactionData_<2168>> {
    static void transform(Context<bp::NormalTransactionData_<2168>> &ctx, bp::NormalTransactionData_<2187> &&from);
};

template <>
struct Transformer<bp::InventoryMismatchData_<2187>, bp::InventoryMismatchData_<2168>> {
    static void transform(Context<bp::InventoryMismatchData_<2168>> &ctx, bp::InventoryMismatchData_<2187> &&from);
};

template <>
struct Transformer<bp::ItemUseInventoryTransaction_<2187>, bp::ItemUseInventoryTransaction_<2168>> {
    static void transform(Context<bp::ItemUseInventoryTransaction_<2168>> &ctx,
                          bp::ItemUseInventoryTransaction_<2187> &&from);
};

template <>
struct Transformer<bp::ItemUseOnActorInventoryTransaction_<2187>, bp::ItemUseOnActorInventoryTransaction_<2168>> {
    static void transform(Context<bp::ItemUseOnActorInventoryTransaction_<2168>> &ctx,
                          bp::ItemUseOnActorInventoryTransaction_<2187> &&from);
};

template <>
struct Transformer<bp::ItemReleaseInventoryTransaction_<2187>, bp::ItemReleaseInventoryTransaction_<2168>> {
    static void transform(Context<bp::ItemReleaseInventoryTransaction_<2168>> &ctx,
                          bp::ItemReleaseInventoryTransaction_<2187> &&from);
};

template <>
struct Transformer<bp::TransactionData_<2187>, bp::TransactionData_<2168>> {
    static void transform(Context<bp::TransactionData_<2168>> &ctx, bp::TransactionData_<2187> &&from);
};

template <>
struct Transformer<bp::InventoryTransactionPacket_<2187>, bp::InventoryTransactionPacket_<2168>> {
    static void transform(Context<bp::InventoryTransactionPacket_<2168>> &ctx,
                          bp::InventoryTransactionPacket_<2187> &&from);
};

} // namespace endweave
