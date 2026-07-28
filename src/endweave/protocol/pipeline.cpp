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
}

std::expected<std::optional<std::string_view>, std::error_code> ProtocolPipeline::transform(Direction direction,
                                                                                            int packet_id,
                                                                                            UserConnection &connection,
                                                                                            std::string_view payload)
{
    std::string_view current = payload;
    std::size_t scratch = 0;

    for (const Pipe &pipe : direction == Direction::Serverbound ? pipes_ : reversed_pipes_) {
        if (!pipe.protocol->hasMapping(pipe.slot, packet_id)) {
            continue;
        }

        std::string &out = scratch_[scratch];
        out.clear(); // BinaryWriter appends
        bedrock::protocol::BinaryReader in{current};
        bedrock::protocol::BinaryWriter writer{out};

        auto action = pipe.protocol->transform(pipe.slot, packet_id, connection, in, writer);
        if (!action) {
            return std::unexpected(action.error());
        }
        if (action.value() == PacketAction::Cancelled) {
            return std::nullopt;
        }

        current = out;
        scratch ^= 1;
    }

    return current;
}

} // namespace endweave
