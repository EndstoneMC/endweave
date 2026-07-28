#pragma once

#include "endweave/protocol/error.h"
#include "endweave/protocol/packet.h"

#include <expected>
#include <functional>
#include <utility>

namespace endweave {

class UserConnection;

/**
 * A packet handler, over the packet in flight rather than a PacketWrapper. Cancelling is
 * PacketError::Cancelled, and a handler that cannot do its job returns its own error.
 *
 * @see ViaVersion PacketHandler, which is void and reports both through exceptions.
 */
using PacketHandler = std::function<std::expected<void, PacketError>(UserConnection &, PacketHolder &)>;

/**
 * One packet's converter between the two versions a step joins. It returns the next era's
 * packet, PacketError::Cancelled to drop it, or the reason it could not convert.
 *
 * @note endweave-specific: ViaVersion converts field by field over a PacketWrapper (map(Type),
 * read, create, ValueTransformer). A generated struct per era makes a converter a plain function
 * from one era's struct to the next one's.
 * @see ViaVersion PacketHandler#handle, whose CancelException and InformativeException are the
 * two things this error channel carries.
 */
template <class In, class Out>
using PacketConverter = std::expected<Out, PacketError> (*)(UserConnection &, const In &);

/**
 * Wraps a converter as a table entry: it decodes In, converts, and leaves Out behind for the
 * stages after it.
 *
 * @note endweave-specific: the bridge from a typed converter to the type-erased handler table,
 * which ViaVersion does not need because every one of its handlers takes a PacketWrapper.
 */
template <class In, class Out>
PacketHandler makePacketHandler(PacketConverter<In, Out> converter)
{
    static_assert(In::Id == Out::Id, "a converter maps one packet id onto itself");
    return [converter](UserConnection &connection, PacketHolder &packet) -> std::expected<void, PacketError> {
        auto in = packet.get<In>();
        if (!in) {
            return std::unexpected(in.error());
        }
        auto out = converter(connection, *in.value());
        if (!out) {
            return std::unexpected(out.error());
        }
        packet.set(std::move(out.value()));
        return {};
    };
}

/**
 * ViaVersion's PacketHandlers DSL, minus the value model.
 *
 * @note The field converters (ViaVersion's map(Type), create, read, ValueTransformer ...) have
 * no counterpart: a converter is a whole-packet function, and a packet no stage rewrites is
 * forwarded as it arrived, which is what passthrough() amounted to.
 *
 * @see ViaVersion PacketHandlers.
 */
namespace PacketHandlers {

/**
 * Drops the packet, without decoding it.
 *
 * @see ViaVersion PacketWrapper#cancel.
 */
inline PacketHandler cancel()
{
    return [](UserConnection &, PacketHolder &) -> std::expected<void, PacketError> {
        return std::unexpected(PacketError::Cancelled);
    };
}

/**
 * Chains two handlers over the same packet.
 *
 * @see ViaVersion PacketHandler#then.
 */
inline PacketHandler then(PacketHandler first, PacketHandler second)
{
    return [first = std::move(first), second = std::move(second)](
               UserConnection &connection, PacketHolder &packet) -> std::expected<void, PacketError> {
        auto result = first(connection, packet);
        if (!result) {
            return result;
        }
        return second(connection, packet);
    };
}

} // namespace PacketHandlers

} // namespace endweave
