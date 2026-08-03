#include "endweave/protocol/pipeline.h"

#include "endweave/protocol/protocol.h"

#include <bedrock/stream.hpp>
#include <utility>

namespace endweave {

ProtocolPipeline::ProtocolPipeline(std::vector<const AbstractProtocol *> base_protocols)
    : base_protocols_(std::move(base_protocols))
{
    rebuild();
}

void ProtocolPipeline::add(const ProtocolPath &path)
{
    path_.insert(path_.end(), path.begin(), path.end());
    rebuild();
}

void ProtocolPipeline::rebuild()
{
    pipes_.clear();
    reversed_pipes_.clear();
    pipes_.reserve(base_protocols_.size() + path_.size());
    reversed_pipes_.reserve(base_protocols_.size() + path_.size());

    for (const AbstractProtocol *protocol : base_protocols_) {
        pipes_.push_back({protocol, slotOf(Direction::Serverbound)});
        reversed_pipes_.push_back({protocol, slotOf(Direction::Clientbound)});
    }
    for (const ProtocolPathEntry &entry : path_) {
        pipes_.push_back({entry.protocol, slotOf(entry.step)});
    }
    for (auto entry = path_.rbegin(); entry != path_.rend(); ++entry) {
        reversed_pipes_.push_back({entry->protocol, slotOf(invert(entry->step))});
    }

    const auto mark = [this](Direction direction, const std::vector<Pipe> &pipes) {
        std::bitset<kPacketIdCount> &handled = handled_[slotOf(direction)];
        handled.reset();
        for (const Pipe &pipe : pipes) {
            for (int packet_id = 0; packet_id < kPacketIdCount; ++packet_id) {
                if (pipe.protocol->hasMapping(pipe.slot, packet_id)) {
                    handled.set(static_cast<std::size_t>(packet_id));
                }
            }
        }
    };
    mark(Direction::Serverbound, pipes_);
    mark(Direction::Clientbound, reversed_pipes_);
}

std::expected<std::optional<std::string_view>, PacketError> ProtocolPipeline::transform(Direction direction,
                                                                                        int packet_id,
                                                                                        UserConnection &connection,
                                                                                        std::string_view payload)
{
    if (!handles(direction, packet_id)) {
        return payload;
    }

    PacketHolder packet{payload};

    for (const Pipe &pipe : direction == Direction::Serverbound ? pipes_ : reversed_pipes_) {
        if (!pipe.protocol->hasMapping(pipe.slot, packet_id)) {
            continue;
        }

        auto result = pipe.protocol->transform(pipe.slot, packet_id, connection, packet);
        if (!result) {
            if (result.error() == PacketError::Cancelled) {
                return std::nullopt;
            }
            return std::unexpected(result.error());
        }
    }

    if (!packet.isDecoded()) {
        return payload;
    }

    scratch_.clear(); // BinaryWriter appends
    bedrock::protocol::BinaryWriter writer{scratch_};
    packet.serialize(writer);
    return scratch_;
}

} // namespace endweave
