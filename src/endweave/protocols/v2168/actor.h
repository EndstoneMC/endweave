#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/actor.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::DataItemEntry_<2168>, bp::DataItemEntry_<1001>> {
    static void transform(Context<bp::DataItemEntry_<1001>> &ctx, bp::DataItemEntry_<2168> &&from);
};

template <>
struct Transformer<bp::SynchedActorData_<2168>::CopyableDataList, bp::SynchedActorData_<1001>::CopyableDataList> {
    static void transform(Context<bp::SynchedActorData_<1001>::CopyableDataList> &ctx,
                          bp::SynchedActorData_<2168>::CopyableDataList &&from);
};

template <>
struct Transformer<bp::AddActorPacket_<2168>, bp::AddActorPacket_<1001>> {
    static void transform(Context<bp::AddActorPacket_<1001>> &ctx, bp::AddActorPacket_<2168> &&from);
};

template <>
struct Transformer<bp::SetLastHurtByPacket_<2168>, bp::SetLastHurtByPacket_<1001>> {
    static void transform(Context<bp::SetLastHurtByPacket_<1001>> &ctx, bp::SetLastHurtByPacket_<2168> &&from);
};

} // namespace endweave
