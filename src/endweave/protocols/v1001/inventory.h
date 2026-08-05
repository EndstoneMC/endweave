#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/inventory.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::NetworkItemStackDescriptor, bp::SerializedNetworkItemStackDescriptor_<2168>> {
    static bp::SerializedNetworkItemStackDescriptor_<2168> transform(bp::NetworkItemStackDescriptor &&from);
};

template <>
struct Transformer<bp::SerializedNetworkItemStackDescriptor_<1001>, bp::SerializedNetworkItemStackDescriptor_<2168>> {
    static bp::SerializedNetworkItemStackDescriptor_<2168> transform(
        bp::SerializedNetworkItemStackDescriptor_<1001> &&from);
};

template <>
struct Transformer<bp::InventoryAction_<1001>, bp::InventoryAction_<2168>> {
    static bp::InventoryAction_<2168> transform(bp::InventoryAction_<1001> &&from);
};

template <>
struct Transformer<bp::InventoryTransaction_<1001>, bp::InventoryTransaction_<2168>> {
    static bp::InventoryTransaction_<2168> transform(bp::InventoryTransaction_<1001> &&from);
};

template <>
struct Transformer<bp::NormalTransactionData_<1001>, bp::NormalTransactionData_<2168>> {
    static bp::NormalTransactionData_<2168> transform(bp::NormalTransactionData_<1001> &&from);
};

template <>
struct Transformer<bp::InventoryMismatchData_<1001>, bp::InventoryMismatchData_<2168>> {
    static bp::InventoryMismatchData_<2168> transform(bp::InventoryMismatchData_<1001> &&from);
};

template <>
struct Transformer<bp::ItemUseInventoryTransaction_<1001>, bp::ItemUseInventoryTransaction_<2168>> {
    static bp::ItemUseInventoryTransaction_<2168> transform(bp::ItemUseInventoryTransaction_<1001> &&from);
};

template <>
struct Transformer<bp::ItemUseOnActorInventoryTransaction_<1001>, bp::ItemUseOnActorInventoryTransaction_<2168>> {
    static bp::ItemUseOnActorInventoryTransaction_<2168> transform(
        bp::ItemUseOnActorInventoryTransaction_<1001> &&from);
};

template <>
struct Transformer<bp::ItemReleaseInventoryTransaction_<1001>, bp::ItemReleaseInventoryTransaction_<2168>> {
    static bp::ItemReleaseInventoryTransaction_<2168> transform(bp::ItemReleaseInventoryTransaction_<1001> &&from);
};

template <>
struct Transformer<bp::TransactionData_<1001>, bp::TransactionData_<2168>> {
    static bp::TransactionData_<2168> transform(bp::TransactionData_<1001> &&from);
};

template <>
struct Transformer<bp::InventoryTransactionPacket_<1001>, bp::InventoryTransactionPacket_<2168>> {
    static bp::InventoryTransactionPacket_<2168> transform(bp::InventoryTransactionPacket_<1001> &&from);
};

template <>
struct Transformer<bp::InventoryContentPacket_<1001>, bp::InventoryContentPacket_<2168>> {
    static bp::InventoryContentPacket_<2168> transform(bp::InventoryContentPacket_<1001> &&from);
};

template <>
struct Transformer<bp::InventorySlotPacket_<1001>, bp::InventorySlotPacket_<2168>> {
    static bp::InventorySlotPacket_<2168> transform(bp::InventorySlotPacket_<1001> &&from);
};

} // namespace endweave
