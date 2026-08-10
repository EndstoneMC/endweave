#include "endweave/protocols/v1001/actor.h"

#include <cstddef>
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

void Transformer<bp::DataItemEntry_<1001>, bp::DataItemEntry_<2168>>::transform(Context<bp::DataItemEntry_<2168>> &ctx,
                                                                                bp::DataItemEntry_<1001> &&from)
{
    auto &to = ctx.out();
    to.id = from.id;
    to.payload = upgradePayload<decltype(to.payload)>(
        from.payload, std::make_index_sequence<std::variant_size_v<decltype(from.payload)>>{});
}

void Transformer<bp::SynchedActorData_<1001>::CopyableDataList, bp::SynchedActorData_<2168>::CopyableDataList>::
    transform(Context<bp::SynchedActorData_<2168>::CopyableDataList> &ctx,
              bp::SynchedActorData_<1001>::CopyableDataList &&from)
{
    auto &to = ctx.out();
    to.data = ew::transform(ctx, std::move(from.data));
}

void Transformer<bp::AddActorPacket_<1001>, bp::AddActorPacket_<2168>>::transform(
    Context<bp::AddActorPacket_<2168>> &ctx, bp::AddActorPacket_<1001> &&from)
{
    auto &to = ctx.out();
    to.entity_id = from.entity_id;
    to.runtime_id = from.runtime_id;
    to.actor_type = std::move(from.actor_type);
    to.pos = from.pos;
    to.velocity = from.velocity;
    to.rot = from.rot;
    to.y_head_rotation = from.y_head_rotation;
    to.y_body_rotation = from.y_body_rotation;
    to.attributes = std::move(from.attributes);
    to.data = ew::transform(ctx, std::move(from.data));
    to.synched_properties = std::move(from.synched_properties);
    to.links = std::move(from.links);
}

void Transformer<bp::SetActorDataPacket_<1001>, bp::SetActorDataPacket_<2168>>::transform(
    Context<bp::SetActorDataPacket_<2168>> &ctx, bp::SetActorDataPacket_<1001> &&from)
{
    auto &to = ctx.out();
    to.id = from.id;
    to.packed_items = ew::transform(ctx, std::move(from.packed_items));
    to.synched_properties = std::move(from.synched_properties);
    to.tick = from.tick;
}

void Transformer<bp::SetLastHurtByPacket_<1001>, bp::SetLastHurtByPacket_<2168>>::transform(
    Context<bp::SetLastHurtByPacket_<2168>> &ctx, bp::SetLastHurtByPacket_<1001> &&from)
{
    auto &to = ctx.out();
    // ENDWEAVE: ActorType composes a category with a type index, and 2168 recategorised the sulfur
    // cube from MONSTER to PATHFINDER_MOB. It is the only member whose number moved.
    to.last_hurt_by = from.last_hurt_by == bp::ActorType_<1001>::SULFUR_CUBE
                        ? bp::ActorType_<2168>::SULFUR_CUBE
                        : static_cast<bp::ActorType_<2168>>(from.last_hurt_by);
}

} // namespace endweave
