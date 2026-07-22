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
 * A base protocol carries slotOf(Direction), a version node slotOf(Step), so the walk needs no
 * direction of its own.
 */
struct Pipe {
    const AbstractProtocol *protocol = nullptr;
    std::size_t slot = 0;
};

/**
 * A connection's chain of protocols, mirroring ViaVersion's ProtocolPipeline.
 *
 * Seeded with the base protocols and filled in once the client's version is known. Two
 * orderings are kept: serverbound runs the path forwards, clientbound runs it backwards with
 * every step inverted. Base protocols sit at the head of both and are never reversed.
 *
 * @note Runs on the server thread. Nothing here is synchronised.
 */
class ProtocolPipeline {
public:
    /**
     * Constructs a pipeline holding only the base protocols.
     *
     * @param base_protocols The always-on protocols, in registration order.
     */
    explicit ProtocolPipeline(std::vector<const AbstractProtocol *> base_protocols);

    /**
     * Adds a resolved path to the pipeline.
     *
     * @param path The path, in serverbound order.
     */
    void add(const ProtocolPath &path);

    /**
     * Gets the stages a serverbound packet passes through.
     *
     * @return The stages, in order.
     */
    [[nodiscard]] const std::vector<Pipe> &pipes() const
    {
        return pipes_;
    }

    /**
     * Gets the stages a clientbound packet passes through.
     *
     * @return The stages, in order.
     */
    [[nodiscard]] const std::vector<Pipe> &reversedPipes() const
    {
        return reversed_pipes_;
    }

    /**
     * Returns whether any version translation happens on this connection.
     *
     * @return true if the pipeline holds more than the base protocols.
     */
    [[nodiscard]] bool hasNonBaseProtocols() const
    {
        return !path_.empty();
    }

    /**
     * Threads a packet body through every stage that handles the given id. Each stage gets a
     * fresh reader over the previous stage's output, the analogue of ViaVersion resetting the
     * reader between protocols.
     *
     * @note The returned view aliases the input when no stage rewrote, and a buffer owned by
     * this pipeline otherwise. Compare data() to tell them apart; it does not survive the next
     * call.
     *
     * @param direction Which way the packet is travelling.
     * @param packet_id The packet id.
     * @param connection The connection the packet belongs to.
     * @param payload The packet body, excluding the header.
     * @return The body to forward, std::nullopt if a stage cancelled, or a codec error.
     */
    std::expected<std::optional<std::string_view>, std::error_code> transform(Direction direction, int packet_id,
                                                                              UserConnection &connection,
                                                                              std::string_view payload);

private:
    void rebuild();

    std::vector<const AbstractProtocol *> base_protocols_;
    ProtocolPath path_;
    std::vector<Pipe> pipes_;
    std::vector<Pipe> reversed_pipes_;
    // Ping-ponged between stages and reused across packets, so a steady stream allocates
    // nothing once the buffers have grown.
    std::array<std::string, 2> scratch_;
};

} // namespace endweave
