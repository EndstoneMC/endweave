#pragma once

#include "endweave/protocol/identical.h"
#include "endweave/protocol/packet.h"
#include "endweave/protocol/rewrite.h"
#include "endweave/protocol/transform.h"
#include "endweave/protocol/version.h"
#include "endweave/protocols/rewriters.h"
#include "endweave/protocols/v2168_to_v2192/transform.h"
#include "endweave/protocols/v2192_to_v2168/transform.h"

#include <array>
#include <bedrock/protocol.hpp>
#include <bedrock/protocol/serializer.hpp>
#include <bedrock/protocol/stream.hpp>
#include <cstddef>
#include <expected>
#include <span>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

namespace bp = bedrock::protocol;

namespace endweave {

namespace detail {

inline constexpr int kSkipBegin = static_cast<int>(bp::MinecraftPacketIds::TitleSpecificPacketsStart);
inline constexpr int kSkipEnd = static_cast<int>(bp::MinecraftPacketIds::TitleSpecificPacketsEnd);

template <int V>
inline constexpr int kPacketIdCount = static_cast<int>(bp::MinecraftPacketIds_<V>::EndId);

template <int From, int To, int Id>
inline constexpr bool path_has_packet_v = has_packet_v<From, Id> && path_has_packet_v<step(From, To), To, Id>;

template <int V, int Id>
inline constexpr bool path_has_packet_v<V, V, Id> = has_packet_v<V, Id>;

template <int From, int To, int Id>
inline constexpr bool should_cancel_v =
    !(Id >= kSkipBegin && Id <= kSkipEnd) && has_packet_v<From, Id> && !path_has_packet_v<From, To, Id>;

template <int From, int To, int Id>
inline constexpr bool should_translate_v =
    From != To && !(Id >= kSkipBegin && Id <= kSkipEnd) && path_has_packet_v<From, To, Id> &&
    (Rewritable<From, To, Id> || !std::is_same_v<packet_of_t<From, Id>, packet_of_t<To, Id>>);

template <int Cur, int To, int Id>
void chain(const Context<packet_of_t<To, Id>> &ctx, packet_of_t<Cur, Id> &&from)
{
    if constexpr (Cur == To) {
        ctx.out() = std::move(from);
    }
    else if constexpr (std::is_same_v<packet_of_t<Cur, Id>, packet_of_t<step(Cur, To), Id>>) {
        chain<step(Cur, To), To, Id>(ctx, std::move(from));
    }
    else {
        packet_of_t<step(Cur, To), Id> next;
        endweave::transform_into(ctx, std::move(from), next);
        // Stop if a hop cancelled the packet.
        if (ctx.isCancelled()) {
            return;
        }
        chain<step(Cur, To), To, Id>(ctx, std::move(next));
    }
}

template <int From, int To, int Id>
std::expected<void, std::error_code> handle(Session &session, bool &cancelled, bp::BinaryReader &in,
                                            bp::BinaryWriter &out)
{
    auto result = bp::deserialize<packet_of_t<From, Id>>(in);
    if (!result) {
        return std::unexpected(result.error());
    }
    // Leftover bytes mean the schema only models part of the packet.
    if (in.getUnreadLength() != 0) {
        return std::unexpected(std::make_error_code(std::errc::protocol_error));
    }

    std::string translated;
    bp::BinaryWriter writer{translated};
    auto &&packet = std::move(result).value();
    if constexpr (Rewritable<From, To, Id>) {
        Rewriter<From, To, Id>::rewrite(packet);
    }
    // Already rewritten for To, so an unchanged type is re-encoded as is.
    if constexpr (std::is_same_v<packet_of_t<From, Id>, packet_of_t<To, Id>>) {
        bp::serialize(writer, packet);
    }
    else {
        packet_of_t<To, Id> translated_packet;
        chain<From, To, Id>(Context<packet_of_t<To, Id>>{session, cancelled, translated_packet}, std::move(packet));
        // Discard whatever was written before a cancel.
        if (cancelled) {
            return {};
        }
        bp::serialize(writer, translated_packet);
    }

    // Read it back to catch a serializer and deserializer that disagree.
    bp::BinaryReader back{translated};
    const auto check = bp::deserialize<packet_of_t<To, Id>>(back);
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

/** How a translator handles a packet id, so the caller can skip ids that need nothing. */
enum class Action : char {
    Passthrough = 0,
    Translate = 1,
    Cancel = 2,
};

/** @see ViaVersion PacketHandler. */
using PacketHandler = std::expected<void, std::error_code> (*)(Session &, bool &, bp::BinaryReader &,
                                                               bp::BinaryWriter &);

namespace detail {

template <int From, int To, int Id>
consteval PacketHandler handlerFor()
{
    if constexpr (should_cancel_v<From, To, Id>) {
        return &cancel;
    }
    else if constexpr (should_translate_v<From, To, Id>) {
        return &handle<From, To, Id>;
    }
    else {
        return nullptr;
    }
}

template <int From, int To, int... Ids>
consteval std::array<PacketHandler, sizeof...(Ids)> makeHandlers(std::integer_sequence<int, Ids...>)
{
    return {handlerFor<From, To, Ids>()...};
}

template <int From, int To>
inline constexpr auto kHandlers = makeHandlers<From, To>(std::make_integer_sequence<int, kPacketIdCount<From>>{});

} // namespace detail

/** The handler table for one From-to-To pair, built at compile time and shared by every connection.
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

template <int From>
constexpr Translator getTranslator(int to)
{
    Translator translator;
    ProtocolVersions::visit(to, [&]<int To>() {
        translator = Translator{kHandlers<From, To>};
    });
    return translator;
}

} // namespace detail

/** @see Velocity StateRegistry.PacketRegistry#getProtocolRegistry. */
constexpr Translator getTranslator(int from, int to)
{
    Translator translator;
    ProtocolVersions::visit(from, [&]<int From>() {
        translator = detail::getTranslator<From>(to);
    });
    return translator;
}

} // namespace endweave
