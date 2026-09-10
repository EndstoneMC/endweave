#pragma once

#include "endweave/protocol/cancel.h"
#include "endweave/protocol/identical.h"
#include "endweave/protocol/rewrite.h"
#include "endweave/protocol/transform.h"
#include "endweave/protocol/version.h"
#include "endweave/protocols/rewriters.h"
#include "endweave/protocols/v2168_to_v2192/transform.h"
#include "endweave/protocols/v2192_to_v2168/transform.h"

#include <array>
#include <bedrock/protocol.hpp>
#include <bedrock/protocol/packet.hpp>
#include <bedrock/protocol/serializer.hpp>
#include <bedrock/protocol/stream.hpp>
#include <concepts>
#include <cstddef>
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

template <ProtocolVersion From, ProtocolVersion To, int Id>
concept Rewritable = requires(packet_of<From, Id> &packet) {
    { Rewriter<From, To, Id>::rewrite(packet) } -> std::same_as<void>;
};

namespace detail {

inline constexpr int kSkipBegin = static_cast<int>(bp::MinecraftPacketIds::TitleSpecificPacketsStart);
inline constexpr int kSkipEnd = static_cast<int>(bp::MinecraftPacketIds::TitleSpecificPacketsEnd);

template <ProtocolVersion V>
inline constexpr int kPacketIdCount = static_cast<int>(bp::MinecraftPacketIds_<static_cast<int>(V)>::EndId);

template <ProtocolVersion From, ProtocolVersion To, int Id>
consteval bool pathHasPacket()
{
    if constexpr (!has_packet<From, Id>) {
        return false;
    }
    else if constexpr (From == To) {
        return true;
    }
    else {
        return pathHasPacket<step(From, To), To, Id>();
    }
}

template <ProtocolVersion From, ProtocolVersion To, int Id>
consteval bool cancelledOnPath()
{
    if constexpr (From == To) {
        return false;
    }
    else {
        return cancel_v<From, step(From, To), Id> || cancelledOnPath<step(From, To), To, Id>();
    }
}

template <ProtocolVersion From, ProtocolVersion To, int Id>
consteval bool shouldCancel()
{
    if constexpr (Id >= kSkipBegin && Id <= kSkipEnd) {
        return false;
    }
    else if constexpr (!has_packet<From, Id>) {
        return false;
    }
    else {
        return !pathHasPacket<From, To, Id>() || cancel_v<From, To, Id> || cancelledOnPath<From, To, Id>();
    }
}

template <ProtocolVersion From, ProtocolVersion To, int Id>
consteval bool rewrites()
{
    return Rewritable<From, To, Id>;
}

template <ProtocolVersion From, ProtocolVersion To, int Id>
consteval bool shouldHandle()
{
    if constexpr (From == To || (Id >= kSkipBegin && Id <= kSkipEnd)) {
        return false;
    }
    else if constexpr (!pathHasPacket<From, To, Id>() || shouldCancel<From, To, Id>()) {
        return false;
    }
    else {
        return rewrites<From, To, Id>() || !wire_equal_v<packet_of<From, Id>, packet_of<To, Id>>;
    }
}

template <ProtocolVersion Cur, ProtocolVersion To, int Id>
void chain(const Context<packet_of<To, Id>> &ctx, packet_of<Cur, Id> &&from)
{
    if constexpr (Cur == To) {
        ctx.out() = std::move(from);
    }
    else if constexpr (std::is_same_v<packet_of<Cur, Id>, packet_of<step(Cur, To), Id>>) {
        chain<step(Cur, To), To, Id>(ctx, std::move(from));
    }
    else {
        static_assert(!WireCompatible<packet_of<Cur, Id>, packet_of<step(Cur, To), Id>>::value,
                      "endweave: this hop is declared wire-compatible, yet the chain has to hold the packet as an "
                      "object across it -- a hop further along that reshapes forces that. Write the Transformer for "
                      "this pair and drop the WireCompatible.");
        packet_of<step(Cur, To), Id> next;
        endweave::transform_into(ctx, std::move(from), next);
        // A hop that dropped the packet leaves nothing worth carrying to the one after it.
        if (ctx.isCancelled()) {
            return;
        }
        chain<step(Cur, To), To, Id>(ctx, std::move(next));
    }
}

template <ProtocolVersion From, ProtocolVersion To, int Id>
std::expected<void, std::error_code> handle(Session &session, bool &cancelled, bp::BinaryReader &in,
                                            bp::BinaryWriter &out)
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
    if constexpr (Rewritable<From, To, Id>) {
        Rewriter<From, To, Id>::rewrite(packet);
    }
    // The rewrite already made the packet mean what To expects, so a pair that encodes the
    // same bytes needs no destination struct and no Transformer.
    if constexpr (wire_equal_v<packet_of<From, Id>, packet_of<To, Id>>) {
        bp::serialize(writer, packet);
    }
    else {
        packet_of<To, Id> translated_packet;
        chain<From, To, Id>(Context<packet_of<To, Id>>{session, cancelled, translated_packet}, std::move(packet));
        // A transform anywhere on the chain may have dropped the packet, and whatever it wrote
        // into the destination before that means nothing.
        if (cancelled) {
            return {};
        }
        bp::serialize(writer, translated_packet);
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

/** @see ViaVersion PacketWrapper#cancel. */
inline std::expected<void, std::error_code> cancel(Session &, bool &cancelled, bp::BinaryReader &, bp::BinaryWriter &)
{
    cancelled = true;
    return {};
}

} // namespace detail

/** What a packet costs on a translator, before any buffer is built. The caller answers this by
 * id first, so a packet that needs nothing never crosses back into the engine. */
enum class Action : char {
    Passthrough = 0,
    Translate = 1,
    Cancel = 2,
};

/** @see ViaVersion PacketHandler. */
using PacketHandler = std::expected<void, std::error_code> (*)(Session &, bool &, bp::BinaryReader &,
                                                               bp::BinaryWriter &);

namespace detail {

template <ProtocolVersion From, ProtocolVersion To, int Id>
consteval PacketHandler handlerFor()
{
    if constexpr (shouldCancel<From, To, Id>()) {
        return &cancel;
    }
    else if constexpr (shouldHandle<From, To, Id>()) {
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

} // namespace detail

/** One From-to-To translation, resolved once and then indexed. The table is a compile-time
 * constant over the pair, so every connection between the same two versions shares it.
 * @see ViaVersion Protocol, Velocity StateRegistry.PacketRegistry.ProtocolRegistry. */
class Translator {
public:
    constexpr Translator() = default;

    constexpr explicit Translator(std::span<const PacketHandler> handlers) : handlers_(handlers) {}

    [[nodiscard]] constexpr PacketHandler get(int id) const
    {
        return id >= 0 && std::cmp_less(id, handlers_.size()) ? handlers_[id] : nullptr;
    }

    [[nodiscard]] constexpr Action getAction(int id) const
    {
        const PacketHandler handler = get(id);
        if (handler == nullptr) {
            return Action::Passthrough;
        }
        return handler == &detail::cancel ? Action::Cancel : Action::Translate;
    }

    [[nodiscard]] constexpr std::size_t size() const
    {
        return handlers_.size();
    }

private:
    std::span<const PacketHandler> handlers_;
};

namespace detail {

template <ProtocolVersion From>
constexpr Translator getTranslator(ProtocolVersion to)
{
    Translator translator;
    ProtocolVersions::visit(to, [&]<ProtocolVersion To>() {
        translator = Translator{kHandlers<From, To>};
    });
    return translator;
}

} // namespace detail

/** @see Velocity StateRegistry.PacketRegistry#getProtocolRegistry. */
constexpr Translator getTranslator(ProtocolVersion from, ProtocolVersion to)
{
    Translator translator;
    ProtocolVersions::visit(from, [&]<ProtocolVersion From>() {
        translator = detail::getTranslator<From>(to);
    });
    return translator;
}

} // namespace endweave
