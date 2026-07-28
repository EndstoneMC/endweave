#pragma once

#include <bedrock/stream.hpp>
#include <expected>
#include <functional>
#include <system_error>
#include <utility>

namespace endweave {

class UserConnection;

/**
 * What a handler did with its packet.
 *
 * @see ViaVersion PacketWrapper#cancel / CancelException, folded into a return value.
 */
enum class PacketAction {
    Translated,
    Cancelled
};

/**
 * A packet handler, over the codec rather than a PacketWrapper.
 *
 * @see ViaVersion PacketHandler.
 */
using PacketHandler = std::function<std::expected<PacketAction, std::error_code>(
    UserConnection &, bedrock::protocol::BinaryReader &, bedrock::protocol::BinaryWriter &)>;

/**
 * ViaVersion's PacketHandlers DSL, minus the value model.
 *
 * @note The field converters (ViaVersion's map(Type), create, read, ValueTransformer ...) are
 * the translation bit and are deliberately absent. They will be rebuilt on the bedrock-protocol
 * codec, not ViaVersion's PacketWrapper. Until then a node registers nothing and packets pass
 * through untouched.
 *
 * @see ViaVersion PacketHandlers.
 */
namespace PacketHandlers {

/**
 * Drops the packet.
 *
 * @see ViaVersion PacketWrapper#cancel.
 */
inline PacketHandler cancel()
{
    return [](UserConnection &, bedrock::protocol::BinaryReader &,
              bedrock::protocol::BinaryWriter &) -> std::expected<PacketAction, std::error_code> {
        return PacketAction::Cancelled;
    };
}

/**
 * Copies every still-unread input byte across.
 *
 * @see ViaVersion PacketWrapper#passthrough.
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
 * Chains two handlers over the same reader and writer.
 *
 * @see ViaVersion PacketHandler#then.
 */
inline PacketHandler then(PacketHandler first, PacketHandler second)
{
    return [first = std::move(first), second = std::move(second)](
               UserConnection &connection, bedrock::protocol::BinaryReader &in,
               bedrock::protocol::BinaryWriter &out) -> std::expected<PacketAction, std::error_code> {
        auto action = first(connection, in, out);
        if (!action || action.value() == PacketAction::Cancelled) {
            return action;
        }
        return second(connection, in, out);
    };
}

} // namespace PacketHandlers

} // namespace endweave
