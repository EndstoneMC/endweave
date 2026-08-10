#include "endweave/protocols/v2181/network.h"

#include <bedrock/enum.hpp>
#include <utility>

namespace endweave {

bp::DisconnectPacket_<2168> Transformer<bp::DisconnectPacket_<2181>, bp::DisconnectPacket_<2168>>::transform(
    bp::DisconnectPacket_<2181> &&from)
{
    using Reason = bp::DisconnectFailReason_<2168>;
    bp::DisconnectPacket_<2168> to;
    // ENDWEAVE: MissingStructureData is 2181's alone and has no 2168 reason, so it lands on Unknown --
    // the message below still says why in words.
    to.reason = bp::enum_cast<Reason>(bp::enum_name(from.reason)).value_or(Reason::UNKNOWN);
    to.messages = std::move(from.messages);
    return to;
}

bp::PacketViolationWarningPacket_<2168> Transformer<
    bp::PacketViolationWarningPacket_<2181>,
    bp::PacketViolationWarningPacket_<2168>>::transform(bp::PacketViolationWarningPacket_<2181> &&from)
{
    using Ids = bp::MinecraftPacketIds_<2168>;
    bp::PacketViolationWarningPacket_<2168> to;
    to.violation_type = from.violation_type;
    to.violation_severity = from.violation_severity;
    // ENDWEAVE: SetPlayerFurnaceOptions is 2181's alone, so a complaint about it names no 2168 packet
    // and falls back to the sentinel past the last id.
    to.violating_packet_id = bp::enum_cast<Ids>(bp::enum_name(from.violating_packet_id)).value_or(Ids::END_ID);
    to.violation_context = std::move(from.violation_context);
    return to;
}

} // namespace endweave
