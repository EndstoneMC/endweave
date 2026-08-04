#pragma once

#include "endweave/protocol/transform.h"
#include "endweave/protocols/v2168/inventory.h"

#include <protocol/equipment.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::MobEquipmentPacket_<2168>> {
    static bp::MobEquipmentPacket_<1001> downgrade(bp::MobEquipmentPacket_<2168> &&from);
};

template <>
struct Transformer<bp::MobArmorEquipmentPacket_<2168>> {
    static bp::MobArmorEquipmentPacket_<1001> downgrade(bp::MobArmorEquipmentPacket_<2168> &&from);
};

} // namespace endweave
