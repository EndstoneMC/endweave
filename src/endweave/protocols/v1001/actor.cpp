#include "endweave/protocols/v1001/actor.h"

#include "endweave/protocols/v1001/sound.h"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <variant>

namespace ew = endweave;

namespace endweave {

namespace {

// ENDWEAVE: 2168 restates the payload type beside the union tag it already writes.
// DataItemType runs in variant order, so the variant index is that value.
template <class To, class From, std::size_t... I>
To upgradePayload(From &from, std::index_sequence<I...>)
{
    To to;
    ((from.index() == I ? static_cast<void>(to = std::variant_alternative_t<I, To>{static_cast<bp::DataItemType>(I),
                                                                                   std::move(std::get<I>(from).value)})
                        : static_cast<void>(0)),
     ...);
    return to;
}

} // namespace

bp::DataItemEntry_<2168> Transformer<bp::DataItemEntry_<1001>, bp::DataItemEntry_<2168>>::transform(
    bp::DataItemEntry_<1001> &&from)
{
    bp::DataItemEntry_<2168> to;
    to.id = from.id;
    to.payload = upgradePayload<decltype(to.payload)>(
        from.payload, std::make_index_sequence<std::variant_size_v<decltype(from.payload)>>{});
    // ENDWEAVE: this key holds a LevelSoundEvent, whose Undefined sentinel is renumbered every
    // version. Left alone, an actor meaning "no heartbeat sound" names a real one at the other
    // end and plays it every HEARTBEAT_INTERVAL_TICKS.
    if (to.id == static_cast<std::uint32_t>(bp::ActorDataIDs::HEARTBEAT_SOUND_EVENT)) {
        if (auto *const sound = std::get_if<bp::DataItemIntPayload_<2168>>(&to.payload)) {
            sound->value = static_cast<std::int32_t>(
                ew::transform_to<bp::LevelSoundEvent_<2168>>(static_cast<bp::LevelSoundEvent_<1001>>(sound->value)));
        }
    }
    return to;
}

bp::SynchedActorData_<2168>::CopyableDataList Transformer<
    bp::SynchedActorData_<1001>::CopyableDataList,
    bp::SynchedActorData_<2168>::CopyableDataList>::transform(bp::SynchedActorData_<1001>::CopyableDataList &&from)
{
    bp::SynchedActorData_<2168>::CopyableDataList to;
    to.data = ew::transform(std::move(from.data));
    return to;
}

bp::AddActorPacket_<2168> Transformer<bp::AddActorPacket_<1001>, bp::AddActorPacket_<2168>>::transform(
    bp::AddActorPacket_<1001> &&from)
{
    bp::AddActorPacket_<2168> to;
    to.entity_id = from.entity_id;
    to.runtime_id = from.runtime_id;
    to.actor_type = std::move(from.actor_type);
    to.pos = from.pos;
    to.velocity = from.velocity;
    to.rot = from.rot;
    to.y_head_rotation = from.y_head_rotation;
    to.y_body_rotation = from.y_body_rotation;
    to.attributes = std::move(from.attributes);
    to.data = ew::transform(std::move(from.data));
    to.synched_properties = std::move(from.synched_properties);
    to.links = std::move(from.links);
    return to;
}

bp::SetActorDataPacket_<2168> Transformer<bp::SetActorDataPacket_<1001>, bp::SetActorDataPacket_<2168>>::transform(
    bp::SetActorDataPacket_<1001> &&from)
{
    bp::SetActorDataPacket_<2168> to;
    to.id = from.id;
    to.packed_items = ew::transform(std::move(from.packed_items));
    to.synched_properties = std::move(from.synched_properties);
    to.tick = from.tick;
    return to;
}

bp::SetLastHurtByPacket_<2168> Transformer<bp::SetLastHurtByPacket_<1001>, bp::SetLastHurtByPacket_<2168>>::transform(
    bp::SetLastHurtByPacket_<1001> &&from)
{
    bp::SetLastHurtByPacket_<2168> to;
    // ENDWEAVE: ActorType composes a category with a type index, and 2168 recategorised the sulfur
    // cube from MONSTER to PATHFINDER_MOB. It is the only member whose number moved.
    to.last_hurt_by = from.last_hurt_by == bp::ActorType_<1001>::SULFUR_CUBE
                        ? bp::ActorType_<2168>::SULFUR_CUBE
                        : static_cast<bp::ActorType_<2168>>(from.last_hurt_by);
    return to;
}

} // namespace endweave
