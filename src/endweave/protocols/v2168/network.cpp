#include "endweave/protocols/v2168/network.h"

#include <bedrock/enum.hpp>
#include <utility>

namespace endweave {

bp::DisconnectPacket_<2181> Transformer<bp::DisconnectPacket_<2168>, bp::DisconnectPacket_<2181>>::transform(
    bp::DisconnectPacket_<2168> &&from)
{
    using Reason = bp::DisconnectFailReason_<2181>;
    bp::DisconnectPacket_<2181> to;
    // ENDWEAVE: 2181 inserted MissingStructureData ahead of the trailing sentinel, so the name carries
    // the reason rather than the byte.
    to.reason = bp::enum_cast<Reason>(bp::enum_name(from.reason)).value_or(Reason::UNKNOWN);
    to.messages = std::move(from.messages);
    return to;
}

bp::PacketViolationWarningPacket_<2181> Transformer<
    bp::PacketViolationWarningPacket_<2168>,
    bp::PacketViolationWarningPacket_<2181>>::transform(bp::PacketViolationWarningPacket_<2168> &&from)
{
    using Ids = bp::MinecraftPacketIds_<2181>;
    bp::PacketViolationWarningPacket_<2181> to;
    to.violation_type = from.violation_type;
    to.violation_severity = from.violation_severity;
    // ENDWEAVE: the ids themselves are stable, but END_ID moved once SetPlayerFurnaceOptions took 351,
    // so the offending packet is named rather than renumbered.
    to.violating_packet_id =
        bp::enum_cast<Ids>(bp::enum_name(from.violating_packet_id)).value_or(Ids::END_ID);
    to.violation_context = std::move(from.violation_context);
    return to;
}

} // namespace endweave
