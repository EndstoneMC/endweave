#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/actor.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::DataItemEntry_<1001>, bp::DataItemEntry_<2168>> {
    static void transform(Context<bp::DataItemEntry_<2168>> &ctx, bp::DataItemEntry_<1001> &&from);
};

template <>
struct Transformer<bp::SynchedActorData_<1001>::CopyableDataList, bp::SynchedActorData_<2168>::CopyableDataList> {
    static void transform(Context<bp::SynchedActorData_<2168>::CopyableDataList> &ctx,
                          bp::SynchedActorData_<1001>::CopyableDataList &&from);
};

template <>
struct Transformer<bp::AddActorPacket_<1001>, bp::AddActorPacket_<2168>> {
    static void transform(Context<bp::AddActorPacket_<2168>> &ctx, bp::AddActorPacket_<1001> &&from);
};

template <>
struct Transformer<bp::SetLastHurtByPacket_<1001>, bp::SetLastHurtByPacket_<2168>> {
    static void transform(Context<bp::SetLastHurtByPacket_<2168>> &ctx, bp::SetLastHurtByPacket_<1001> &&from);
};

} // namespace endweave
