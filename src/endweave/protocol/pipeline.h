#pragma once

#include "endweave/protocol/transform.h"
#include "endweave/protocol/version.h"
#include "endweave/protocols/v1001/transform.h"
#include "endweave/protocols/v2168/transform.h"

#include <bedrock/packet.hpp>
#include <bedrock/protocol.hpp>
#include <bedrock/serializer.hpp>
#include <bedrock/stream.hpp>
#include <expected>
#include <system_error>
#include <type_traits>
#include <utility>

namespace bp = bedrock::protocol;

namespace endweave {

enum class TranslateResult {
    Passthrough,
    Translated,
};

template <ProtocolVersion V, int Id>
using PacketOf = bp::packet_of_t<static_cast<int>(V), Id>;

template <ProtocolVersion V, int Id>
inline constexpr bool has_packet = bp::has_packet_v<static_cast<int>(V), Id>;

namespace detail {

// 200-299 is the vendor extension range; nothing there is Mojang's to translate.
inline constexpr int kVendorIdBegin = 200;
inline constexpr int kVendorIdEnd = 300;

template <ProtocolVersion V>
inline constexpr int kPacketIdCount = static_cast<int>(bp::MinecraftPacketIds_<static_cast<int>(V)>::END_ID);

template <ProtocolVersion From, ProtocolVersion To, int Id>
consteval bool isPresentOnPath()
{
    if constexpr (!has_packet<From, Id>) {
        return false;
    }
    else if constexpr (From == To) {
        return true;
    }
    else if constexpr (From < To) {
        return isPresentOnPath<next(From), To, Id>();
    }
    else {
        return isPresentOnPath<prev(From), To, Id>();
    }
}

template <ProtocolVersion From, ProtocolVersion To, int Id>
consteval bool isReshapedOnPath()
{
    if constexpr (From == To) {
        return false;
    }
    else {
        constexpr ProtocolVersion node = From < To ? next(From) : prev(From);
        return !std::is_same_v<PacketOf<From, Id>, PacketOf<node, Id>> || isReshapedOnPath<node, To, Id>();
    }
}

template <ProtocolVersion From, ProtocolVersion To, int Id>
consteval bool isTranslatable()
{
    if constexpr (Id >= kVendorIdBegin && Id < kVendorIdEnd) {
        return false;
    }
    else {
        return isPresentOnPath<From, To, Id>() && isReshapedOnPath<From, To, Id>();
    }
}

template <ProtocolVersion From, ProtocolVersion To, int Id>
PacketOf<To, Id> walk(PacketOf<From, Id> &&value)
{
    if constexpr (From == To) {
        return std::move(value);
    }
    else if constexpr (From < To) {
        constexpr ProtocolVersion node = next(From);
        if constexpr (std::is_same_v<PacketOf<From, Id>, PacketOf<node, Id>>) {
            return walk<node, To, Id>(std::move(value));
        }
        else {
            static_assert(std::is_same_v<decltype(endweave::upgrade(std::move(value))), PacketOf<node, Id>>);
            return walk<node, To, Id>(endweave::upgrade(std::move(value)));
        }
    }
    else {
        constexpr ProtocolVersion node = prev(From);
        if constexpr (std::is_same_v<PacketOf<From, Id>, PacketOf<node, Id>>) {
            return walk<node, To, Id>(std::move(value));
        }
        else {
            static_assert(std::is_same_v<decltype(endweave::downgrade(std::move(value))), PacketOf<node, Id>>);
            return walk<node, To, Id>(endweave::downgrade(std::move(value)));
        }
    }
}

template <ProtocolVersion From, ProtocolVersion To, int Id>
std::expected<TranslateResult, std::error_code> translateOne(bp::BinaryReader &in, bp::BinaryWriter &out)
{
    if constexpr (isTranslatable<From, To, Id>()) {
        auto packet = bp::deserialize<PacketOf<From, Id>>(in);
        if (!packet) {
            return std::unexpected(packet.error());
        }
        bp::serialize(out, walk<From, To, Id>(std::move(packet.value())));
        return TranslateResult::Translated;
    }
    else {
        return TranslateResult::Passthrough;
    }
}

} // namespace detail

template <ProtocolVersion From, ProtocolVersion To>
std::expected<TranslateResult, std::error_code> translate(int id, bp::BinaryReader &in, bp::BinaryWriter &out)
{
    return [&]<int... Ids>(std::integer_sequence<int, Ids...>) {
        std::expected<TranslateResult, std::error_code> result = TranslateResult::Passthrough;
        static_cast<void>(((Ids == id ? (result = detail::translateOne<From, To, Ids>(in, out), true) : false) || ...));
        return result;
    }(std::make_integer_sequence<int, detail::kPacketIdCount<From>>{});
}

using Translator = std::expected<TranslateResult, std::error_code> (*)(int, bp::BinaryReader &, bp::BinaryWriter &);

namespace detail {

template <ProtocolVersion From>
constexpr Translator resolveTo(ProtocolVersion to)
{
    Translator translator = nullptr;
    ProtocolVersions::visit(to, [&]<ProtocolVersion To>() {
        translator = &translate<From, To>;
    });
    return translator;
}

} // namespace detail

/** @see Velocity StateRegistry.PacketRegistry#getProtocolRegistry. */
constexpr Translator getTranslator(ProtocolVersion from, ProtocolVersion to)
{
    Translator translator = nullptr;
    ProtocolVersions::visit(from, [&]<ProtocolVersion From>() {
        translator = detail::resolveTo<From>(to);
    });
    return translator;
}

} // namespace endweave
