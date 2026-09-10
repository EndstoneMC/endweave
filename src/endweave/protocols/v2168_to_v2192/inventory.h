#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/inventory.h>
#include <bedrock/protocol/transaction.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::ItemUseInventoryTransaction_<2168>::ActionType,
                   bp::ItemUseInventoryTransaction_<2192>::ActionType> {
    static void transform(Context<bp::ItemUseInventoryTransaction_<2192>::ActionType> &ctx,
                          bp::ItemUseInventoryTransaction_<2168>::ActionType &&from);
};

template <>
struct Transformer<bp::ItemUseInventoryTransaction_<2168>, bp::ItemUseInventoryTransaction_<2192>> {
    static void transform(Context<bp::ItemUseInventoryTransaction_<2192>> &ctx,
                          bp::ItemUseInventoryTransaction_<2168> &&from);
};

template <>
struct Transformer<bp::ContainerOpenPacket_<2168>, bp::ContainerOpenPacket_<2192>> {
    static void transform(Context<bp::ContainerOpenPacket_<2192>> &ctx, bp::ContainerOpenPacket_<2168> &&from);
};

template <>
struct Transformer<bp::ContainerClosePacket_<2168>, bp::ContainerClosePacket_<2192>> {
    static void transform(Context<bp::ContainerClosePacket_<2192>> &ctx, bp::ContainerClosePacket_<2168> &&from);
};
} // namespace endweave
