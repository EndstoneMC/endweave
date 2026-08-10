#include "endweave/protocols/v2168/actor.h"

#include <cstddef>
#include <utility>
#include <variant>

namespace ew = endweave;

namespace endweave {

namespace {

// ENDWEAVE: The restated DataItemType is dropped; 1001 reads the payload off the union tag,
// which survives.
template <class To, class From, std::size_t... I>
To downgradePayload(From &from, std::index_sequence<I...>)
{
    To to;
    ((from.index() == I ? static_cast<void>(to = std::variant_alternative_t<I, To>{std::move(std::get<I>(from).value)})
                        : static_cast<void>(0)),
     ...);
    return to;
}

} // namespace

bp::DataItemEntry_<1001> Transformer<bp::DataItemEntry_<2168>, bp::DataItemEntry_<1001>>::transform(
    bp::DataItemEntry_<2168> &&from)
{
    bp::DataItemEntry_<1001> to;
    to.id = from.id;
    to.payload = downgradePayload<decltype(to.payload)>(
        from.payload, std::make_index_sequence<std::variant_size_v<decltype(from.payload)>>{});
    return to;
}

bp::SynchedActorData_<1001>::CopyableDataList Transformer<
    bp::SynchedActorData_<2168>::CopyableDataList,
    bp::SynchedActorData_<1001>::CopyableDataList>::transform(bp::SynchedActorData_<2168>::CopyableDataList &&from)
{
    bp::SynchedActorData_<1001>::CopyableDataList to;
    to.data = ew::transform(std::move(from.data));
    return to;
}

bp::AddActorPacket_<1001> Transformer<bp::AddActorPacket_<2168>, bp::AddActorPacket_<1001>>::transform(
    bp::AddActorPacket_<2168> &&from)
{
    bp::AddActorPacket_<1001> to;
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

bp::SetActorDataPacket_<1001> Transformer<bp::SetActorDataPacket_<2168>, bp::SetActorDataPacket_<1001>>::transform(
    bp::SetActorDataPacket_<2168> &&from)
{
    bp::SetActorDataPacket_<1001> to;
    to.id = from.id;
    to.packed_items = ew::transform(std::move(from.packed_items));
    to.synched_properties = std::move(from.synched_properties);
    to.tick = from.tick;
    return to;
}

bp::SetLastHurtByPacket_<1001> Transformer<bp::SetLastHurtByPacket_<2168>, bp::SetLastHurtByPacket_<1001>>::transform(
    bp::SetLastHurtByPacket_<2168> &&from)
{
    bp::SetLastHurtByPacket_<1001> to;
    // ENDWEAVE: the sulfur cube moves back to its MONSTER-composed number, and the cushion has no
    // 1001 counterpart at all, so it lands on UNDEFINED rather than naming another actor.
    switch (from.last_hurt_by) {
    case bp::ActorType_<2168>::SULFUR_CUBE:
        to.last_hurt_by = bp::ActorType_<1001>::SULFUR_CUBE;
        break;
    case bp::ActorType_<2168>::CUSHION:
        to.last_hurt_by = bp::ActorType_<1001>::UNDEFINED;
        break;
    default:
        to.last_hurt_by = static_cast<bp::ActorType_<1001>>(from.last_hurt_by);
        break;
    }
    return to;
}

} // namespace endweave
