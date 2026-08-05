#include "endweave/protocols/v1001/equipment.h"

namespace ew = endweave;

namespace endweave {

bp::MobEquipmentPacket_<2168> Transformer<bp::MobEquipmentPacket_<1001>, bp::MobEquipmentPacket_<2168>>::transform(
    bp::MobEquipmentPacket_<1001> &&from)
{
    bp::MobEquipmentPacket_<2168> to;
    to.runtime_id = from.runtime_id;
    to.item = ew::transform(std::move(from.item));
    to.slot = from.slot;
    to.selected_slot = from.selected_slot;
    to.container_id = from.container_id;
    return to;
}

bp::MobArmorEquipmentPacket_<2168> Transformer<bp::MobArmorEquipmentPacket_<1001>, bp::MobArmorEquipmentPacket_<2168>>::
    transform(bp::MobArmorEquipmentPacket_<1001> &&from)
{
    bp::MobArmorEquipmentPacket_<2168> to;
    to.runtime_id = from.runtime_id;
    to.head = ew::transform(std::move(from.head));
    to.torso = ew::transform(std::move(from.torso));
    to.legs = ew::transform(std::move(from.legs));
    to.feet = ew::transform(std::move(from.feet));
    to.body = ew::transform(std::move(from.body));
    return to;
}

} // namespace endweave
