#include "endweave/protocols/v2168/equipment.h"

namespace ew = endweave;

namespace endweave {

bp::MobEquipmentPacket_<1001> Transformer<bp::MobEquipmentPacket_<2168>>::downgrade(
    bp::MobEquipmentPacket_<2168> &&from)
{
    bp::MobEquipmentPacket_<1001> to;
    to.runtime_id = from.runtime_id;
    to.item = ew::downgrade(from.item);
    to.slot = from.slot;
    to.selected_slot = from.selected_slot;
    to.container_id = from.container_id;
    return to;
}

bp::MobArmorEquipmentPacket_<1001> Transformer<bp::MobArmorEquipmentPacket_<2168>>::downgrade(
    bp::MobArmorEquipmentPacket_<2168> &&from)
{
    bp::MobArmorEquipmentPacket_<1001> to;
    to.runtime_id = from.runtime_id;
    to.head = ew::downgrade(from.head);
    to.torso = ew::downgrade(from.torso);
    to.legs = ew::downgrade(from.legs);
    to.feet = ew::downgrade(from.feet);
    to.body = ew::downgrade(from.body);
    return to;
}

} // namespace endweave
