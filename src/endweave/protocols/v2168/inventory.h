#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/inventory.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::SerializedNetworkItemStackDescriptor_<2168>> {
    static bp::SerializedNetworkItemStackDescriptor_<1001> downgrade(
        bp::SerializedNetworkItemStackDescriptor_<2168> &&from);
};

template <>
struct Transformer<bp::InventoryAction_<2168>> {
    static bp::InventoryAction_<1001> downgrade(bp::InventoryAction_<2168> &&from);
};

template <>
struct Transformer<bp::InventoryTransaction_<2168>> {
    static bp::InventoryTransaction_<1001> downgrade(bp::InventoryTransaction_<2168> &&from);
};

template <>
struct Transformer<bp::NormalTransactionData_<2168>> {
    static bp::NormalTransactionData_<1001> downgrade(bp::NormalTransactionData_<2168> &&from);
};

template <>
struct Transformer<bp::InventoryMismatchData_<2168>> {
    static bp::InventoryMismatchData_<1001> downgrade(bp::InventoryMismatchData_<2168> &&from);
};

template <>
struct Transformer<bp::ItemUseInventoryTransaction_<2168>> {
    static bp::ItemUseInventoryTransaction_<1001> downgrade(bp::ItemUseInventoryTransaction_<2168> &&from);
};

template <>
struct Transformer<bp::ItemUseOnActorInventoryTransaction_<2168>> {
    static bp::ItemUseOnActorInventoryTransaction_<1001> downgrade(
        bp::ItemUseOnActorInventoryTransaction_<2168> &&from);
};

template <>
struct Transformer<bp::ItemReleaseInventoryTransaction_<2168>> {
    static bp::ItemReleaseInventoryTransaction_<1001> downgrade(bp::ItemReleaseInventoryTransaction_<2168> &&from);
};

template <>
struct Transformer<bp::TransactionData_<2168>> {
    static bp::TransactionData_<1001> downgrade(bp::TransactionData_<2168> &&from);
};

template <>
struct Transformer<bp::InventoryTransactionPacket_<2168>> {
    static bp::InventoryTransactionPacket_<1001> downgrade(bp::InventoryTransactionPacket_<2168> &&from);
};

template <>
struct Transformer<bp::InventoryContentPacket_<2168>> {
    static bp::InventoryContentPacket_<1001> downgrade(bp::InventoryContentPacket_<2168> &&from);
};

template <>
struct Transformer<bp::InventorySlotPacket_<2168>> {
    static bp::InventorySlotPacket_<1001> downgrade(bp::InventorySlotPacket_<2168> &&from);
};

} // namespace endweave
