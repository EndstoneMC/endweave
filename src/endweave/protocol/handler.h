#pragma once

#include <bedrock/serializer.hpp>
#include <bedrock/stream.hpp>
#include <expected>
#include <functional>
#include <optional>
#include <system_error>
#include <utility>

namespace endweave {

class UserConnection;

/**
 * What a handler did with its packet. Cancelled stands in for ViaVersion's
 * PacketWrapper::cancel() and drops the packet without forwarding anything.
 */
enum class PacketAction {
    Translated,
    Cancelled
};

/**
 * ViaVersion's PacketHandler, over the codec rather than a PacketWrapper: read the
 * source-version body off the reader, write the target-version body to the writer.
 */
using PacketHandler = std::function<std::expected<PacketAction, std::error_code>(
    UserConnection &, bedrock::protocol::BinaryReader &, bedrock::protocol::BinaryWriter &)>;

/**
 * ViaVersion's PacketHandlers DSL, rebuilt on the generated codec.
 */
namespace PacketHandlers {

/**
 * Wraps a typed converter: deserialize From, convert, serialize To.
 *
 * @note Name the source version -- `map<PacketV975>(&upgrade)`. upgrade is overloaded per
 * packet, so nothing else pins down which overload is meant.
 *
 * @param convert The converter.
 * @return A handler that always translates.
 */
template <class From, class To>
PacketHandler map(To (*convert)(const From &))
{
    return [convert](UserConnection &, bedrock::protocol::BinaryReader &in,
                     bedrock::protocol::BinaryWriter &out) -> std::expected<PacketAction, std::error_code> {
        auto packet = bedrock::protocol::Serializer<From>::deserialize(in);
        if (!packet) {
            return std::unexpected(packet.error());
        }
        bedrock::protocol::Serializer<To>::serialize(out, convert(*packet));
        return PacketAction::Translated;
    };
}

/**
 * Wraps a converter that may refuse, for a form with no spelling at the target version: a
 * returned nullopt cancels the packet.
 *
 * @param convert The converter.
 * @return A handler that translates or cancels.
 */
template <class From, class To>
PacketHandler map(std::optional<To> (*convert)(const From &))
{
    return [convert](UserConnection &, bedrock::protocol::BinaryReader &in,
                     bedrock::protocol::BinaryWriter &out) -> std::expected<PacketAction, std::error_code> {
        auto packet = bedrock::protocol::Serializer<From>::deserialize(in);
        if (!packet) {
            return std::unexpected(packet.error());
        }
        auto converted = convert(*packet);
        if (!converted) {
            return PacketAction::Cancelled;
        }
        bedrock::protocol::Serializer<To>::serialize(out, *converted);
        return PacketAction::Translated;
    };
}

/**
 * Wraps a typed converter that needs per-connection state.
 *
 * @param convert The converter.
 * @return A handler that always translates.
 */
template <class From, class To>
PacketHandler map(To (*convert)(UserConnection &, const From &))
{
    return [convert](UserConnection &connection, bedrock::protocol::BinaryReader &in,
                     bedrock::protocol::BinaryWriter &out) -> std::expected<PacketAction, std::error_code> {
        auto packet = bedrock::protocol::Serializer<From>::deserialize(in);
        if (!packet) {
            return std::unexpected(packet.error());
        }
        bedrock::protocol::Serializer<To>::serialize(out, convert(connection, *packet));
        return PacketAction::Translated;
    };
}

/**
 * Drops the packet -- the handler ViaVersion's cancelClientbound registers.
 *
 * @return A handler that always cancels.
 */
inline PacketHandler cancel()
{
    return [](UserConnection &, bedrock::protocol::BinaryReader &,
              bedrock::protocol::BinaryWriter &) -> std::expected<PacketAction, std::error_code> {
        return PacketAction::Cancelled;
    };
}

/**
 * Forwards every still-unread input byte, ViaVersion's trailing passthrough.
 *
 * @return A handler that copies the rest of the body across.
 */
inline PacketHandler passthrough()
{
    return [](UserConnection &, bedrock::protocol::BinaryReader &in,
              bedrock::protocol::BinaryWriter &out) -> std::expected<PacketAction, std::error_code> {
        out.writeRawBytes(in.getView().substr(in.getReadPointer()));
        return PacketAction::Translated;
    };
}

/**
 * ViaVersion's PacketHandler.then: chains two handlers over the same reader and writer.
 *
 * @param first Runs first.
 * @param second Runs on whatever the first left unread.
 * @return The combined handler.
 */
inline PacketHandler then(PacketHandler first, PacketHandler second)
{
    return [first = std::move(first), second = std::move(second)](
               UserConnection &connection, bedrock::protocol::BinaryReader &in,
               bedrock::protocol::BinaryWriter &out) -> std::expected<PacketAction, std::error_code> {
        auto action = first(connection, in, out);
        if (!action || *action == PacketAction::Cancelled) {
            return action;
        }
        return second(connection, in, out);
    };
}

} // namespace PacketHandlers

} // namespace endweave
