#pragma once

#include "endweave/protocol/transform.h"
#include "endweave/protocol/version.h"
#include "endweave/protocols/v1001/transform.h"
#include "endweave/protocols/v2168/transform.h"

#include <array>
#include <bedrock/packet.hpp>
#include <bedrock/protocol.hpp>
#include <bedrock/serializer.hpp>
#include <bedrock/stream.hpp>
#include <expected>
#include <span>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

namespace bp = bedrock::protocol;

namespace endweave {

template <ProtocolVersion V, int Id>
using packet_of = bp::packet_of_t<static_cast<int>(V), Id>;

template <ProtocolVersion V, int Id>
inline constexpr bool has_packet = bp::has_packet_v<static_cast<int>(V), Id>;

namespace detail {

inline constexpr int kSkipBegin = static_cast<int>(bp::MinecraftPacketIds::TITLE_SPECIFIC_PACKETS_START);
inline constexpr int kSkipEnd = static_cast<int>(bp::MinecraftPacketIds::TITLE_SPECIFIC_PACKETS_END);

template <ProtocolVersion V>
inline constexpr int kPacketIdCount = static_cast<int>(bp::MinecraftPacketIds_<static_cast<int>(V)>::END_ID);

template <ProtocolVersion From, ProtocolVersion To, int Id>
consteval bool shouldHandle()
{
    if constexpr (Id >= kSkipBegin && Id <= kSkipEnd) {
        return false;
    }
    else {
        return has_packet<From, Id> && has_packet<To, Id> && !std::is_same_v<packet_of<From, Id>, packet_of<To, Id>>;
    }
}

template <ProtocolVersion From, ProtocolVersion To, int Id>
consteval bool shouldCancel()
{
    if constexpr (Id >= kSkipBegin && Id <= kSkipEnd) {
        return false;
    }
    else {
        return has_packet<From, Id> && !has_packet<To, Id>;
    }
}

template <ProtocolVersion From, ProtocolVersion To, int Id>
std::expected<void, std::error_code> handle(bp::BinaryReader &in, bp::BinaryWriter &out)
{
    auto result = bp::deserialize<packet_of<From, Id>>(in);
    if (!result) {
        return std::unexpected(result.error());
    }
    // A schema that models only part of a packet still deserialises, and the transform then
    // builds its answer from half a source. The bytes left over are the only sign of it.
    if (in.getUnreadLength() != 0) {
        return std::unexpected(std::make_error_code(std::errc::protocol_error));
    }

    std::string translated;
    bp::BinaryWriter writer{translated};
    auto &&packet = std::move(result).value();
    if constexpr (From < To) {
        static_assert(std::is_same_v<decltype(endweave::upgrade(packet)), packet_of<To, Id>>);
        bp::serialize(writer, endweave::upgrade(packet));
    }
    else {
        static_assert(std::is_same_v<decltype(endweave::downgrade(packet)), packet_of<To, Id>>);
        bp::serialize(writer, endweave::downgrade(packet));
    }

    // The destination has to be able to read back what was just written for it. Serialiser
    // and deserialiser are generated apart, so nothing else holds the pair to each other.
    bp::BinaryReader back{translated};
    const auto check = bp::deserialize<packet_of<To, Id>>(back);
    if (!check) {
        return std::unexpected(check.error());
    }
    if (back.getUnreadLength() != 0) {
        return std::unexpected(std::make_error_code(std::errc::bad_message));
    }

    out.writeRawBytes(translated);
    return {};
}

} // namespace detail

/** @see ViaVersion PacketHandler. */
using PacketHandler = std::expected<void, std::error_code> (*)(bp::BinaryReader &, bp::BinaryWriter &);

namespace detail {

template <ProtocolVersion From, ProtocolVersion To, int Id>
consteval PacketHandler handlerFor()
{
    if constexpr (shouldHandle<From, To, Id>()) {
        return &handle<From, To, Id>;
    }
    else {
        return nullptr;
    }
}

template <ProtocolVersion From, ProtocolVersion To, int... Ids>
consteval std::array<PacketHandler, sizeof...(Ids)> makeHandlers(std::integer_sequence<int, Ids...>)
{
    return {handlerFor<From, To, Ids>()...};
}

template <ProtocolVersion From, ProtocolVersion To>
inline constexpr auto kHandlers = makeHandlers<From, To>(std::make_integer_sequence<int, kPacketIdCount<From>>{});

template <ProtocolVersion From, ProtocolVersion To, int... Ids>
consteval std::array<bool, sizeof...(Ids)> makeCancelled(std::integer_sequence<int, Ids...>)
{
    return {shouldCancel<From, To, Ids>()...};
}

template <ProtocolVersion From, ProtocolVersion To>
inline constexpr auto kCancelled = makeCancelled<From, To>(std::make_integer_sequence<int, kPacketIdCount<From>>{});

} // namespace detail

/** @see ViaVersion PacketHandlers, Velocity StateRegistry.PacketRegistry.ProtocolRegistry. */
class PacketHandlers {
public:
    constexpr PacketHandlers() = default;

    constexpr PacketHandlers(std::span<const PacketHandler> handlers, std::span<const bool> cancelled)
        : handlers_(handlers), cancelled_(cancelled)
    {
    }

    [[nodiscard]] constexpr PacketHandler get(int id) const
    {
        return id >= 0 && std::cmp_less(id, handlers_.size()) ? handlers_[id] : nullptr;
    }

    /** @see ViaVersion Protocol#cancelServerbound, Protocol#cancelClientbound. */
    [[nodiscard]] constexpr bool isCancelled(int id) const
    {
        return id >= 0 && std::cmp_less(id, cancelled_.size()) && cancelled_[id];
    }

private:
    std::span<const PacketHandler> handlers_;
    std::span<const bool> cancelled_;
};

namespace detail {

template <ProtocolVersion From>
constexpr PacketHandlers getPacketHandlers(ProtocolVersion to)
{
    PacketHandlers handlers;
    ProtocolVersions::visit(to, [&]<ProtocolVersion To>() {
        handlers = PacketHandlers{kHandlers<From, To>, kCancelled<From, To>};
    });
    return handlers;
}

} // namespace detail

/** @see Velocity StateRegistry.PacketRegistry#getProtocolRegistry. */
constexpr PacketHandlers getPacketHandlers(ProtocolVersion from, ProtocolVersion to)
{
    PacketHandlers handlers;
    ProtocolVersions::visit(from, [&]<ProtocolVersion From>() {
        handlers = detail::getPacketHandlers<From>(to);
    });
    return handlers;
}

} // namespace endweave
