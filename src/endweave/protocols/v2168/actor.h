#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/actor.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::DataItemEntry_<2168>, bp::DataItemEntry_<1001>> {
    static bp::DataItemEntry_<1001> transform(bp::DataItemEntry_<2168> &&from);
};

template <>
struct Transformer<bp::SynchedActorData_<2168>::CopyableDataList, bp::SynchedActorData_<1001>::CopyableDataList> {
    static bp::SynchedActorData_<1001>::CopyableDataList transform(
        bp::SynchedActorData_<2168>::CopyableDataList &&from);
};

template <>
struct Transformer<bp::AddActorPacket_<2168>, bp::AddActorPacket_<1001>> {
    static bp::AddActorPacket_<1001> transform(bp::AddActorPacket_<2168> &&from);
};

template <>
struct Transformer<bp::SetActorDataPacket_<2168>, bp::SetActorDataPacket_<1001>> {
    static bp::SetActorDataPacket_<1001> transform(bp::SetActorDataPacket_<2168> &&from);
};

template <>
struct Transformer<bp::SetLastHurtByPacket_<2168>, bp::SetLastHurtByPacket_<1001>> {
    static bp::SetLastHurtByPacket_<1001> transform(bp::SetLastHurtByPacket_<2168> &&from);
};

} // namespace endweave
