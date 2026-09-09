#pragma once

#include <array>
#include <cstddef>
#include <utility>

namespace endweave {

/** @see ViaVersion ProtocolVersion. */
enum class ProtocolVersion : int {
    UNKNOWN = -1,
    v26_40 = 2168,
    v26_50 = 2192,
};

namespace ProtocolVersions {

/** @see Velocity ProtocolVersion#SUPPORTED_VERSIONS. */
constexpr std::array SUPPORTED_VERSIONS{
    ProtocolVersion::v26_40,
    ProtocolVersion::v26_50,
};

/** Versions that reach the wire but encode every modelled packet exactly as a supported one does,
 * and so route as it rather than earning handler tables of their own. The claim is not taken on
 * trust: `identical.h` holds each pair against the schema, so a version that stops being identical
 * fails the build there instead of translating as the wrong era. */
constexpr std::array<std::pair<int, ProtocolVersion>, 1> WIRE_IDENTICAL{{
    {2169, ProtocolVersion::v26_40}, // 1.26.45
}};

constexpr std::size_t indexOf(ProtocolVersion version)
{
    for (std::size_t i = 0; i < SUPPORTED_VERSIONS.size(); ++i) {
        if (SUPPORTED_VERSIONS[i] == version) {
            return i;
        }
    }
    return SUPPORTED_VERSIONS.size();
}

/** @see Velocity ProtocolVersion#getProtocolVersion(int). */
constexpr ProtocolVersion getProtocolVersion(int protocol_version)
{
    for (const ProtocolVersion version : SUPPORTED_VERSIONS) {
        if (static_cast<int>(version) == protocol_version) {
            return version;
        }
    }
    for (const auto &[announced, routes_as] : WIRE_IDENTICAL) {
        if (announced == protocol_version) {
            return routes_as;
        }
    }
    return ProtocolVersion::UNKNOWN;
}

template <class F>
constexpr bool visit(ProtocolVersion version, F &&visitor)
{
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
        return (
            (SUPPORTED_VERSIONS[I] == version ? (visitor.template operator()<SUPPORTED_VERSIONS[I]>(), true) : false) ||
            ...);
    }(std::make_index_sequence<SUPPORTED_VERSIONS.size()>{});
}

} // namespace ProtocolVersions

consteval ProtocolVersion step(ProtocolVersion from, ProtocolVersion to)
{
    const std::size_t here = ProtocolVersions::indexOf(from);
    const std::size_t there = ProtocolVersions::indexOf(to);
    if (here == ProtocolVersions::SUPPORTED_VERSIONS.size() || there == ProtocolVersions::SUPPORTED_VERSIONS.size() ||
        here == there) {
        return from;
    }
    return ProtocolVersions::SUPPORTED_VERSIONS[there > here ? here + 1 : here - 1];
}

} // namespace endweave
