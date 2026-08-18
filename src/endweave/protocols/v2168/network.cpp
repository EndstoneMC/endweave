#include "endweave/protocols/v2168/network.h"

#include <bedrock/enum.hpp>
#include <utility>

namespace endweave {

void Transformer<bp::DisconnectPacket_<2168>, bp::DisconnectPacket_<2192>>::transform(
    Context<bp::DisconnectPacket_<2192>> &ctx, bp::DisconnectPacket_<2168> &&from)
{
    using Reason = bp::DisconnectFailReason_<2192>;
    auto &to = ctx.out();
    // ENDWEAVE: 2192 inserted MissingStructureData and UnsupportedTransport ahead of the trailing
    // sentinel, so the name carries the reason rather than the byte.
    to.reason = bp::enum_cast<Reason>(bp::enum_name(from.reason)).value_or(Reason::UNKNOWN);
    to.messages = std::move(from.messages);
}

void Transformer<bp::PacketViolationWarningPacket_<2168>, bp::PacketViolationWarningPacket_<2192>>::transform(
    Context<bp::PacketViolationWarningPacket_<2192>> &ctx, bp::PacketViolationWarningPacket_<2168> &&from)
{
    using Ids = bp::MinecraftPacketIds_<2192>;
    auto &to = ctx.out();
    to.violation_type = from.violation_type;
    to.violation_severity = from.violation_severity;
    // ENDWEAVE: the ids themselves are stable, but END_ID moved once SetPlayerFurnaceOptions and
    // RecordStarted took 351 and 352, so the offending packet is named rather than renumbered.
    to.violating_packet_id = bp::enum_cast<Ids>(bp::enum_name(from.violating_packet_id)).value_or(Ids::END_ID);
    to.violation_context = std::move(from.violation_context);
}

} // namespace endweave
