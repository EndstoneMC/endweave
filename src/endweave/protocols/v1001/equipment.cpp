#include "endweave/protocols/v1001/equipment.h"

namespace ew = endweave;

namespace endweave {

bp::MobEquipmentPacket_<2168> Transformer<bp::MobEquipmentPacket_<1001>>::upgrade(bp::MobEquipmentPacket_<1001> &&from)
{
    bp::MobEquipmentPacket_<2168> to;
    to.runtime_id = from.runtime_id;
    to.item = ew::upgrade(from.item);
    to.slot = from.slot;
    to.selected_slot = from.selected_slot;
    to.container_id = from.container_id;
    return to;
}

bp::MobArmorEquipmentPacket_<2168> Transformer<bp::MobArmorEquipmentPacket_<1001>>::upgrade(
    bp::MobArmorEquipmentPacket_<1001> &&from)
{
    bp::MobArmorEquipmentPacket_<2168> to;
    to.runtime_id = from.runtime_id;
    to.head = ew::upgrade(from.head);
    to.torso = ew::upgrade(from.torso);
    to.legs = ew::upgrade(from.legs);
    to.feet = ew::upgrade(from.feet);
    to.body = ew::upgrade(from.body);
    return to;
}

} // namespace endweave
