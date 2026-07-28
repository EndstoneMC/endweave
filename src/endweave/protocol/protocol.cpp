#include "endweave/protocol/protocol.h"

#include <stdexcept>
#include <utility>

namespace endweave {

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

std::expected<void, PacketError> AbstractProtocol::transform(std::size_t slot, int packet_id,
                                                             UserConnection &connection, PacketHolder &packet) const
{
    return mappings_[slot][static_cast<std::size_t>(packet_id)](connection, packet);
}

void AbstractProtocol::registerAt(std::size_t slot, int packet_id, PacketHandler handler)
{
    std::vector<PacketHandler> &table = mappings_[slot];
    const auto index = static_cast<std::size_t>(packet_id);
    if (index >= table.size()) {
        table.resize(index + 1);
    }
    if (table[index]) {
        throw std::invalid_argument("packet " + std::to_string(index) + " already registered in " + name_ +
                                    "; use append or replace if that is intentional");
    }
    table[index] = std::move(handler);
}

void AbstractProtocol::appendAt(std::size_t slot, int packet_id, PacketHandler handler)
{
    std::vector<PacketHandler> &table = mappings_[slot];
    const auto index = static_cast<std::size_t>(packet_id);
    if (index < table.size() && table[index]) {
        table[index] = PacketHandlers::then(std::move(table[index]), std::move(handler));
        return;
    }
    registerAt(slot, packet_id, std::move(handler));
}

void AbstractProtocol::replaceAt(std::size_t slot, int packet_id, PacketHandler handler)
{
    std::vector<PacketHandler> &table = mappings_[slot];
    const auto index = static_cast<std::size_t>(packet_id);
    if (index >= table.size() || !table[index]) {
        throw std::invalid_argument("packet " + std::to_string(index) + " has no handler to replace in " + name_);
    }
    table[index] = std::move(handler);
}

void AbstractProtocol::cancelUpgrade(bedrock::protocol::MinecraftPacketIds packet_id)
{
    registerAt(slotOf(Step::Upgrade), static_cast<int>(packet_id), PacketHandlers::cancel());
}

void AbstractProtocol::cancelDowngrade(bedrock::protocol::MinecraftPacketIds packet_id)
{
    registerAt(slotOf(Step::Downgrade), static_cast<int>(packet_id), PacketHandlers::cancel());
}

void AbstractProtocol::cancelClientbound(bedrock::protocol::MinecraftPacketIds packet_id)
{
    registerAt(slotOf(Direction::Clientbound), static_cast<int>(packet_id), PacketHandlers::cancel());
}

void AbstractProtocol::cancelServerbound(bedrock::protocol::MinecraftPacketIds packet_id)
{
    registerAt(slotOf(Direction::Serverbound), static_cast<int>(packet_id), PacketHandlers::cancel());
}

} // namespace endweave
