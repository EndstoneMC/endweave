#pragma once

#include "endweave/protocol/direction.h"
#include "endweave/protocol/path.h"

#include <array>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace endweave {

class AbstractProtocol;
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
     * Threads a packet body through every stage that handles the given id.
     *
     * @note The returned view aliases the input when no stage rewrote, and a buffer owned by
     * this pipeline otherwise. It does not survive the next call.
     *
     * @param direction Which way the packet is travelling.
     * @param packet_id The packet id.
     * @param connection The connection the packet belongs to.
     * @param payload The packet body, excluding the header.
     * @return The body to forward, std::nullopt if a stage cancelled, or a codec error.
     * @see ViaVersion ProtocolPipelineImpl#transform.
     */
    std::expected<std::optional<std::string_view>, std::error_code> transform(Direction direction, int packet_id,
                                                                              UserConnection &connection,
                                                                              std::string_view payload);

private:
    /** @see ViaVersion ProtocolPipelineImpl#refreshReversedList. */
    void rebuild();

    std::vector<const AbstractProtocol *> base_protocols_;
    ProtocolPath path_;
    std::vector<Pipe> pipes_;          // ViaVersion: protocolList
    std::vector<Pipe> reversed_pipes_; // ViaVersion: reversedProtocolList
    // endweave-specific codec scratch, ping-ponged between stages and reused across packets.
    std::array<std::string, 2> scratch_;
};

} // namespace endweave
