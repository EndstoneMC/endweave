#pragma once

namespace endweave {

/**
 * The Bedrock packet ids endweave registers handlers for. Only the ids in use are listed, and the
 * value is the on-wire id.
 *
 * @note Id reuse across versions is not modelled yet, so one flat enum serves every version step.
 *
 * @see ViaVersion's per-version ClientboundPacketType / ServerboundPacketType.
 */
enum class PacketIds : int {
    Login = 1,
    Disconnect = 5,
    PacketViolationWarning = 156,
    RequestNetworkSettings = 193,
};

} // namespace endweave
