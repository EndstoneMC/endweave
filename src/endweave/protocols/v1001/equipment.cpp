#include "endweave/protocols/v1001/equipment.h"

namespace ew = endweave;

namespace endweave {

void Transformer<bp::MobEquipmentPacket_<1001>, bp::MobEquipmentPacket_<2168>>::transform(
    Context<bp::MobEquipmentPacket_<2168>> &ctx, bp::MobEquipmentPacket_<1001> &&from)
{
    auto &to = ctx.out();
    to.runtime_id = from.runtime_id;
    to.item = ew::transform(ctx, std::move(from.item));
    to.slot = from.slot;
    to.selected_slot = from.selected_slot;
    to.container_id = from.container_id;
}

void Transformer<bp::MobArmorEquipmentPacket_<1001>, bp::MobArmorEquipmentPacket_<2168>>::transform(
    Context<bp::MobArmorEquipmentPacket_<2168>> &ctx, bp::MobArmorEquipmentPacket_<1001> &&from)
{
    auto &to = ctx.out();
    to.runtime_id = from.runtime_id;
    to.head = ew::transform(ctx, std::move(from.head));
    to.torso = ew::transform(ctx, std::move(from.torso));
    to.legs = ew::transform(ctx, std::move(from.legs));
    to.feet = ew::transform(ctx, std::move(from.feet));
    to.body = ew::transform(ctx, std::move(from.body));
}

} // namespace endweave
