#include "endweave/protocols/v2192/item_stack.h"

#include <utility>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::ItemStackResponseSlotInfo_<2192>, bp::ItemStackResponseSlotInfo_<2168>>::transform(
    Context<bp::ItemStackResponseSlotInfo_<2168>> &ctx, bp::ItemStackResponseSlotInfo_<2192> &&from)
{
    auto &to = ctx.out();
    to.requested_slot = from.requested_slot;
    to.slot = from.slot;
    to.amount = from.amount;
    to.item_stack_net_id = from.item_stack_net_id;
    to.custom_name = std::move(from.custom_name);
    to.durability_correction = from.durability_correction;
}

void Transformer<bp::ItemStackResponseContainerInfo_<2192>, bp::ItemStackResponseContainerInfo_<2168>>::transform(
    Context<bp::ItemStackResponseContainerInfo_<2168>> &ctx, bp::ItemStackResponseContainerInfo_<2192> &&from)
{
    auto &to = ctx.out();
    to.full_container_name = std::move(from.full_container_name);
    to.slots = ew::transform(ctx, std::move(from.slots));
}

void Transformer<bp::ItemStackResponseInfo_<2192>, bp::ItemStackResponseInfo_<2168>>::transform(
    Context<bp::ItemStackResponseInfo_<2168>> &ctx, bp::ItemStackResponseInfo_<2192> &&from)
{
    auto &to = ctx.out();
    to.result = from.result;
    to.client_request_id = from.client_request_id;
    to.containers = ew::transform(ctx, std::move(from.containers));
}

void Transformer<bp::ItemStackResponsePacket_<2192>, bp::ItemStackResponsePacket_<2168>>::transform(
    Context<bp::ItemStackResponsePacket_<2168>> &ctx, bp::ItemStackResponsePacket_<2192> &&from)
{
    auto &to = ctx.out();
    to.responses = ew::transform(ctx, std::move(from.responses));
}

} // namespace endweave
