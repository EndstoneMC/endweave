#include "endweave/protocols/v2187/network.h"

#include <bedrock/enum.hpp>
#include <utility>

namespace endweave {

void Transformer<bp::DisconnectPacket_<2187>, bp::DisconnectPacket_<2168>>::transform(
    Context<bp::DisconnectPacket_<2168>> &ctx, bp::DisconnectPacket_<2187> &&from)
{
    using Reason = bp::DisconnectFailReason_<2168>;
    auto &to = ctx.out();
    // ENDWEAVE: MissingStructureData and UnsupportedTransport are 2187's alone and have no 2168 reason,
    // so they land on Unknown -- the message below still says why in words.
    to.reason = bp::enum_cast<Reason>(bp::enum_name(from.reason)).value_or(Reason::UNKNOWN);
    to.messages = std::move(from.messages);
}

void Transformer<bp::PacketViolationWarningPacket_<2187>, bp::PacketViolationWarningPacket_<2168>>::transform(
    Context<bp::PacketViolationWarningPacket_<2168>> &ctx, bp::PacketViolationWarningPacket_<2187> &&from)
{
    using Ids = bp::MinecraftPacketIds_<2168>;
    auto &to = ctx.out();
    to.violation_type = from.violation_type;
    to.violation_severity = from.violation_severity;
    // ENDWEAVE: SetPlayerFurnaceOptions and RecordStarted are 2187's alone, so a complaint about either
    // names no 2168 packet and falls back to the sentinel past the last id.
    to.violating_packet_id = bp::enum_cast<Ids>(bp::enum_name(from.violating_packet_id)).value_or(Ids::END_ID);
    to.violation_context = std::move(from.violation_context);
}

} // namespace endweave
