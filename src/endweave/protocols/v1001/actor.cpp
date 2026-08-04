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

bp::DataItemEntry_<2168> Transformer<bp::DataItemEntry_<1001>>::upgrade(bp::DataItemEntry_<1001> &&from)
{
    bp::DataItemEntry_<2168> to;
    to.id = from.id;
    to.payload = upgradePayload<decltype(to.payload)>(
        from.payload, std::make_index_sequence<std::variant_size_v<decltype(from.payload)>>{});
    return to;
}

bp::SynchedActorData_<2168>::CopyableDataList Transformer<bp::SynchedActorData_<1001>::CopyableDataList>::upgrade(
    bp::SynchedActorData_<1001>::CopyableDataList &&from)
{
    bp::SynchedActorData_<2168>::CopyableDataList to;
    to.data = ew::upgrade(from.data);
    return to;
}

bp::AddActorPacket_<2168> Transformer<bp::AddActorPacket_<1001>>::upgrade(bp::AddActorPacket_<1001> &&from)
{
    bp::AddActorPacket_<2168> to;
    to.entity_id = from.entity_id;
    to.runtime_id = from.runtime_id;
    to.type = std::move(from.type);
    to.pos = from.pos;
    to.velocity = from.velocity;
    to.rot = from.rot;
    to.y_head_rotation = from.y_head_rotation;
    to.y_body_rotation = from.y_body_rotation;
    to.attributes = std::move(from.attributes);
    to.data = ew::upgrade(from.data);
    to.synched_properties = std::move(from.synched_properties);
    to.links = std::move(from.links);
    return to;
}

bp::SetActorDataPacket_<2168> Transformer<bp::SetActorDataPacket_<1001>>::upgrade(bp::SetActorDataPacket_<1001> &&from)
{
    bp::SetActorDataPacket_<2168> to;
    to.id = from.id;
    to.packed_items = ew::upgrade(from.packed_items);
    to.synched_properties = std::move(from.synched_properties);
    to.tick = from.tick;
    return to;
}

} // namespace endweave
