#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/inventory.h>

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
struct Transformer<bp::InventoryTransaction_<2168>, bp::InventoryTransaction_<1001>> {
    static void transform(Context<bp::InventoryTransaction_<1001>> &ctx, bp::InventoryTransaction_<2168> &&from);
};

template <>
struct Transformer<bp::NormalTransactionData_<2168>, bp::NormalTransactionData_<1001>> {
    static void transform(Context<bp::NormalTransactionData_<1001>> &ctx, bp::NormalTransactionData_<2168> &&from);
};

template <>
struct Transformer<bp::InventoryMismatchData_<2168>, bp::InventoryMismatchData_<1001>> {
    static void transform(Context<bp::InventoryMismatchData_<1001>> &ctx, bp::InventoryMismatchData_<2168> &&from);
};

template <>
struct Transformer<bp::ItemUseInventoryTransaction_<2168>, bp::ItemUseInventoryTransaction_<1001>> {
    static void transform(Context<bp::ItemUseInventoryTransaction_<1001>> &ctx,
                          bp::ItemUseInventoryTransaction_<2168> &&from);
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
struct Transformer<bp::InventoryTransactionPacket_<2168>, bp::InventoryTransactionPacket_<1001>> {
    static void transform(Context<bp::InventoryTransactionPacket_<1001>> &ctx,
                          bp::InventoryTransactionPacket_<2168> &&from);
};

template <>
struct Transformer<bp::InventoryContentPacket_<2168>, bp::InventoryContentPacket_<1001>> {
    static void transform(Context<bp::InventoryContentPacket_<1001>> &ctx, bp::InventoryContentPacket_<2168> &&from);
};

template <>
struct Transformer<bp::InventorySlotPacket_<2168>, bp::InventorySlotPacket_<1001>> {
    static void transform(Context<bp::InventorySlotPacket_<1001>> &ctx, bp::InventorySlotPacket_<2168> &&from);
};

template <>
struct Transformer<bp::InventorySource_<2168>, bp::InventorySource_<2192>> {
    static void transform(Context<bp::InventorySource_<2192>> &ctx, bp::InventorySource_<2168> &&from);
};

template <>
struct Transformer<bp::InventoryAction_<2168>, bp::InventoryAction_<2192>> {
    static void transform(Context<bp::InventoryAction_<2192>> &ctx, bp::InventoryAction_<2168> &&from);
};

template <>
struct Transformer<bp::InventoryTransaction_<2168>, bp::InventoryTransaction_<2192>> {
    static void transform(Context<bp::InventoryTransaction_<2192>> &ctx, bp::InventoryTransaction_<2168> &&from);
};

template <>
struct Transformer<bp::NormalTransactionData_<2168>, bp::NormalTransactionData_<2192>> {
    static void transform(Context<bp::NormalTransactionData_<2192>> &ctx, bp::NormalTransactionData_<2168> &&from);
};

template <>
struct Transformer<bp::InventoryMismatchData_<2168>, bp::InventoryMismatchData_<2192>> {
    static void transform(Context<bp::InventoryMismatchData_<2192>> &ctx, bp::InventoryMismatchData_<2168> &&from);
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

template <>
struct Transformer<bp::InventoryTransactionPacket_<2168>, bp::InventoryTransactionPacket_<2192>> {
    static void transform(Context<bp::InventoryTransactionPacket_<2192>> &ctx,
                          bp::InventoryTransactionPacket_<2168> &&from);
};

} // namespace endweave
