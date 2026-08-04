#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/inventory.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::NetworkItemStackDescriptor> {
    static bp::SerializedNetworkItemStackDescriptor_<2168> upgrade(bp::NetworkItemStackDescriptor &&from);
};

template <>
struct Transformer<bp::SerializedNetworkItemStackDescriptor_<1001>> {
    static bp::SerializedNetworkItemStackDescriptor_<2168> upgrade(
        bp::SerializedNetworkItemStackDescriptor_<1001> &&from);
};

template <>
struct Transformer<bp::InventoryAction_<1001>> {
    static bp::InventoryAction_<2168> upgrade(bp::InventoryAction_<1001> &&from);
};

template <>
struct Transformer<bp::InventoryTransaction_<1001>> {
    static bp::InventoryTransaction_<2168> upgrade(bp::InventoryTransaction_<1001> &&from);
};

template <>
struct Transformer<bp::NormalTransactionData_<1001>> {
    static bp::NormalTransactionData_<2168> upgrade(bp::NormalTransactionData_<1001> &&from);
};

template <>
struct Transformer<bp::InventoryMismatchData_<1001>> {
    static bp::InventoryMismatchData_<2168> upgrade(bp::InventoryMismatchData_<1001> &&from);
};

template <>
struct Transformer<bp::ItemUseInventoryTransaction_<1001>> {
    static bp::ItemUseInventoryTransaction_<2168> upgrade(bp::ItemUseInventoryTransaction_<1001> &&from);
};

template <>
struct Transformer<bp::ItemUseOnActorInventoryTransaction_<1001>> {
    static bp::ItemUseOnActorInventoryTransaction_<2168> upgrade(bp::ItemUseOnActorInventoryTransaction_<1001> &&from);
};

template <>
struct Transformer<bp::ItemReleaseInventoryTransaction_<1001>> {
    static bp::ItemReleaseInventoryTransaction_<2168> upgrade(bp::ItemReleaseInventoryTransaction_<1001> &&from);
};

template <>
struct Transformer<bp::TransactionData_<1001>> {
    static bp::TransactionData_<2168> upgrade(bp::TransactionData_<1001> &&from);
};

template <>
struct Transformer<bp::InventoryTransactionPacket_<1001>> {
    static bp::InventoryTransactionPacket_<2168> upgrade(bp::InventoryTransactionPacket_<1001> &&from);
};

template <>
struct Transformer<bp::InventoryContentPacket_<1001>> {
    static bp::InventoryContentPacket_<2168> upgrade(bp::InventoryContentPacket_<1001> &&from);
};

template <>
struct Transformer<bp::InventorySlotPacket_<1001>> {
    static bp::InventorySlotPacket_<2168> upgrade(bp::InventorySlotPacket_<1001> &&from);
};

} // namespace endweave
