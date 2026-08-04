#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/actor.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::DataItemEntry_<1001>> {
    static bp::DataItemEntry_<2168> upgrade(bp::DataItemEntry_<1001> &&from);
};

template <>
struct Transformer<bp::SynchedActorData_<1001>::CopyableDataList> {
    static bp::SynchedActorData_<2168>::CopyableDataList upgrade(bp::SynchedActorData_<1001>::CopyableDataList &&from);
};

template <>
struct Transformer<bp::AddActorPacket_<1001>> {
    static bp::AddActorPacket_<2168> upgrade(bp::AddActorPacket_<1001> &&from);
};

template <>
struct Transformer<bp::SetActorDataPacket_<1001>> {
    static bp::SetActorDataPacket_<2168> upgrade(bp::SetActorDataPacket_<1001> &&from);
};

} // namespace endweave
