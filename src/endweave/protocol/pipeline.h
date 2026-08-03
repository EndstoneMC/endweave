#pragma once

#include "endweave/protocol/direction.h"
#include "endweave/protocol/error.h"
#include "endweave/protocol/path.h"
#include "endweave/protocol/protocol.h"

#include <array>
#include <bitset>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace endweave {

class UserConnection;

/**
 * One stage of a pipeline: a protocol plus the table to address it by.
 *
 * @note endweave-specific. ViaVersion stores a bare List<Protocol> and picks the table by
 * direction inside transform(). A node picks it by Step, so the slot is pinned per stage here.
 */
struct Pipe {
    const AbstractProtocol *protocol = nullptr;
    std::size_t slot = 0;
};

/**
 * A connection's chain of protocols. Serverbound runs the path forwards. Clientbound runs it
 * backwards with every step inverted. Base protocols lead both and are never reversed.
 *
 * @note Runs on the server thread. Nothing here is synchronised.
 *
 * @see ViaVersion ProtocolPipeline (api) and ProtocolPipelineImpl (common).
 */
class ProtocolPipeline {
public:
    /**
     * @param base_protocols The always-on protocols, in registration order.
     * @see ViaVersion ProtocolPipelineImpl constructor.
     */
    explicit ProtocolPipeline(std::vector<const AbstractProtocol *> base_protocols);

    /**
     * @param path The path to add, in serverbound order.
     * @see ViaVersion ProtocolPipeline#add(Collection).
     */
    void add(const ProtocolPath &path);

    /**
     * @return The stages a serverbound packet passes through, in order.
     * @see ViaVersion ProtocolPipeline#pipes.
     */
    [[nodiscard]] const std::vector<Pipe> &pipes() const
    {
        return pipes_;
    }

    /**
     * @return The stages a clientbound packet passes through, in order.
     * @see ViaVersion ProtocolPipeline#reversedPipes.
     */
    [[nodiscard]] const std::vector<Pipe> &reversedPipes() const
    {
        return reversed_pipes_;
    }

    /**
     * @return true if the pipeline holds more than the base protocols.
     * @see ViaVersion ProtocolPipeline#hasNonBaseProtocols.
     */
    [[nodiscard]] bool hasNonBaseProtocols() const
    {
        return !path_.empty();
    }

    /**
     * @param direction Which way the packet is travelling.
     * @param packet_id The packet id.
     * @return true if some stage in that direction has a handler for the id.
     * @note endweave-specific. ViaVersion asks each protocol in turn inside transform(), which is
     * the scan this replaces.
     */
    [[nodiscard]] bool handles(Direction direction, int packet_id) const
    {
        return packet_id >= 0 && packet_id < kPacketIdCount &&
               handled_[slotOf(direction)].test(static_cast<std::size_t>(packet_id));
    }

    /**
     * Threads a packet through every stage that handles the given id. The first such stage
     * decodes the body, the rest hand the decoded packet along, and it is written back out
     * once at the end. A packet no stage handles is never decoded.
     *
     * @note The returned view aliases the input when no stage handled the id, and a buffer
     * owned by this pipeline otherwise. It does not survive the next call.
     *
     * @param direction Which way the packet is travelling.
     * @param packet_id The packet id.
     * @param connection The connection the packet belongs to.
     * @param payload The packet body, excluding the header.
     * @return The body to forward, std::nullopt if a stage cancelled, or why it could not be
     * translated.
     * @see ViaVersion ProtocolPipelineImpl#transform.
     */
    std::expected<std::optional<std::string_view>, PacketError> transform(Direction direction, int packet_id,
                                                                          UserConnection &connection,
                                                                          std::string_view payload);

private:
    /** @see ViaVersion ProtocolPipelineImpl#refreshReversedList. */
    void rebuild();

    std::vector<const AbstractProtocol *> base_protocols_;
    ProtocolPath path_;
    std::vector<Pipe> pipes_;          // ViaVersion: protocolList
    std::vector<Pipe> reversed_pipes_; // ViaVersion: reversedProtocolList
    // endweave-specific: the union of every stage's mappings, so transform() skips the scan.
    std::array<std::bitset<kPacketIdCount>, 2> handled_;
    // endweave-specific encode scratch, written once per packet and reused across packets.
    std::string scratch_;
};

} // namespace endweave
