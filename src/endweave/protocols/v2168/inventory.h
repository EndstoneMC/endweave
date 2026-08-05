#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/inventory.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::SerializedNetworkItemStackDescriptor_<2168>, bp::SerializedNetworkItemStackDescriptor_<1001>> {
    static bp::SerializedNetworkItemStackDescriptor_<1001> transform(
        bp::SerializedNetworkItemStackDescriptor_<2168> &&from);
};

template <>
struct Transformer<bp::InventoryAction_<2168>, bp::InventoryAction_<1001>> {
    static bp::InventoryAction_<1001> transform(bp::InventoryAction_<2168> &&from);
};

template <>
struct Transformer<bp::InventoryTransaction_<2168>, bp::InventoryTransaction_<1001>> {
    static bp::InventoryTransaction_<1001> transform(bp::InventoryTransaction_<2168> &&from);
};

template <>
struct Transformer<bp::NormalTransactionData_<2168>, bp::NormalTransactionData_<1001>> {
    static bp::NormalTransactionData_<1001> transform(bp::NormalTransactionData_<2168> &&from);
};

template <>
struct Transformer<bp::InventoryMismatchData_<2168>, bp::InventoryMismatchData_<1001>> {
    static bp::InventoryMismatchData_<1001> transform(bp::InventoryMismatchData_<2168> &&from);
};

template <>
struct Transformer<bp::ItemUseInventoryTransaction_<2168>, bp::ItemUseInventoryTransaction_<1001>> {
    static bp::ItemUseInventoryTransaction_<1001> transform(bp::ItemUseInventoryTransaction_<2168> &&from);
};

template <>
struct Transformer<bp::ItemUseOnActorInventoryTransaction_<2168>, bp::ItemUseOnActorInventoryTransaction_<1001>> {
    static bp::ItemUseOnActorInventoryTransaction_<1001> transform(
        bp::ItemUseOnActorInventoryTransaction_<2168> &&from);
};

template <>
struct Transformer<bp::ItemReleaseInventoryTransaction_<2168>, bp::ItemReleaseInventoryTransaction_<1001>> {
    static bp::ItemReleaseInventoryTransaction_<1001> transform(bp::ItemReleaseInventoryTransaction_<2168> &&from);
};

template <>
struct Transformer<bp::TransactionData_<2168>, bp::TransactionData_<1001>> {
    static bp::TransactionData_<1001> transform(bp::TransactionData_<2168> &&from);
};

template <>
struct Transformer<bp::InventoryTransactionPacket_<2168>, bp::InventoryTransactionPacket_<1001>> {
    static bp::InventoryTransactionPacket_<1001> transform(bp::InventoryTransactionPacket_<2168> &&from);
};

template <>
struct Transformer<bp::InventoryContentPacket_<2168>, bp::InventoryContentPacket_<1001>> {
    static bp::InventoryContentPacket_<1001> transform(bp::InventoryContentPacket_<2168> &&from);
};

template <>
struct Transformer<bp::InventorySlotPacket_<2168>, bp::InventorySlotPacket_<1001>> {
    static bp::InventorySlotPacket_<1001> transform(bp::InventorySlotPacket_<2168> &&from);
};

} // namespace endweave
