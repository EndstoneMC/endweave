#include "endweave/protocol/protocol.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace endweave {
namespace {

std::size_t indexOf(MinecraftPacketIds packet_id)
{
    return static_cast<std::size_t>(static_cast<int>(packet_id));
}

} // namespace

AbstractProtocol::AbstractProtocol(std::string name) : name_(std::move(name)) {}

AbstractProtocol::AbstractProtocol(ProtocolVersion version)
    : name_(std::to_string(static_cast<int>(version))), version_(static_cast<int>(version))
{
}

void AbstractProtocol::initialize()
{
    if (initialized_) {
        throw std::logic_error("protocol " + name_ + " has already been initialized");
    }
    initialized_ = true;
    registerPackets();
}

void AbstractProtocol::setPreviousVersion(int version)
{
    previous_ = version;
}

bool AbstractProtocol::hasMapping(std::size_t slot, int packet_id) const
{
    const std::vector<PacketHandler> &table = mappings_[slot];
    if (packet_id < 0 || static_cast<std::size_t>(packet_id) >= table.size()) {
        return false;
    }
    return static_cast<bool>(table[static_cast<std::size_t>(packet_id)]);
}

std::expected<PacketAction, std::error_code> AbstractProtocol::transform(std::size_t slot, int packet_id,
                                                                         UserConnection &connection,
                                                                         bedrock::protocol::BinaryReader &in,
                                                                         bedrock::protocol::BinaryWriter &out) const
{
    // hasMapping() is the caller's fast path; an unmapped id never reaches here.
    return mappings_[slot][static_cast<std::size_t>(packet_id)](connection, in, out);
}

void AbstractProtocol::registerAt(std::size_t slot, MinecraftPacketIds packet_id, PacketHandler handler)
{
    std::vector<PacketHandler> &table = mappings_[slot];
    const std::size_t index = indexOf(packet_id);
    if (index >= table.size()) {
        table.resize(std::max(index + 1, indexOf(MinecraftPacketIds::Count)));
    }
    if (table[index]) {
        throw std::invalid_argument("packet " + std::to_string(index) + " already registered in " + name_ +
                                    "; use append or replace if that is intentional");
    }
    table[index] = std::move(handler);
}

void AbstractProtocol::appendAt(std::size_t slot, MinecraftPacketIds packet_id, PacketHandler handler)
{
    std::vector<PacketHandler> &table = mappings_[slot];
    const std::size_t index = indexOf(packet_id);
    if (index < table.size() && table[index]) {
        table[index] = PacketHandlers::then(std::move(table[index]), std::move(handler));
        return;
    }
    registerAt(slot, packet_id, std::move(handler));
}

void AbstractProtocol::replaceAt(std::size_t slot, MinecraftPacketIds packet_id, PacketHandler handler)
{
    std::vector<PacketHandler> &table = mappings_[slot];
    const std::size_t index = indexOf(packet_id);
    if (index >= table.size() || !table[index]) {
        throw std::invalid_argument("packet " + std::to_string(index) + " has no handler to replace in " + name_);
    }
    table[index] = std::move(handler);
}

void AbstractProtocol::registerUpgrade(MinecraftPacketIds packet_id, PacketHandler handler)
{
    registerAt(slotOf(Step::Upgrade), packet_id, std::move(handler));
}

void AbstractProtocol::registerDowngrade(MinecraftPacketIds packet_id, PacketHandler handler)
{
    registerAt(slotOf(Step::Downgrade), packet_id, std::move(handler));
}

void AbstractProtocol::cancelUpgrade(MinecraftPacketIds packet_id)
{
    registerAt(slotOf(Step::Upgrade), packet_id, PacketHandlers::cancel());
}

void AbstractProtocol::cancelDowngrade(MinecraftPacketIds packet_id)
{
    registerAt(slotOf(Step::Downgrade), packet_id, PacketHandlers::cancel());
}

void AbstractProtocol::appendUpgrade(MinecraftPacketIds packet_id, PacketHandler handler)
{
    appendAt(slotOf(Step::Upgrade), packet_id, std::move(handler));
}

void AbstractProtocol::appendDowngrade(MinecraftPacketIds packet_id, PacketHandler handler)
{
    appendAt(slotOf(Step::Downgrade), packet_id, std::move(handler));
}

void AbstractProtocol::replaceUpgrade(MinecraftPacketIds packet_id, PacketHandler handler)
{
    replaceAt(slotOf(Step::Upgrade), packet_id, std::move(handler));
}

void AbstractProtocol::replaceDowngrade(MinecraftPacketIds packet_id, PacketHandler handler)
{
    replaceAt(slotOf(Step::Downgrade), packet_id, std::move(handler));
}

void AbstractProtocol::registerClientbound(MinecraftPacketIds packet_id, PacketHandler handler)
{
    registerAt(slotOf(Direction::Clientbound), packet_id, std::move(handler));
}

void AbstractProtocol::registerServerbound(MinecraftPacketIds packet_id, PacketHandler handler)
{
    registerAt(slotOf(Direction::Serverbound), packet_id, std::move(handler));
}

void AbstractProtocol::cancelClientbound(MinecraftPacketIds packet_id)
{
    registerAt(slotOf(Direction::Clientbound), packet_id, PacketHandlers::cancel());
}

void AbstractProtocol::cancelServerbound(MinecraftPacketIds packet_id)
{
    registerAt(slotOf(Direction::Serverbound), packet_id, PacketHandlers::cancel());
}

} // namespace endweave
