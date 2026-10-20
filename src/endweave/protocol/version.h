#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <utility>

namespace endweave {

namespace ProtocolVersions {

/** @see Velocity ProtocolVersion#SUPPORTED_VERSIONS. */
constexpr std::array SUPPORTED_VERSIONS{
    2168, // 26.40
    2193, // 26.50
    2225, // 26.60
};

/** Versions that encode every packet the same as a supported version and are routed as it.
 * `handler.h` checks each entry at compile time. */
constexpr std::array<std::pair<int, int>, 1> WIRE_IDENTICAL{{
    {2169, 2168}, // 26.45
}};

constexpr std::size_t indexOf(int version)
{
    for (std::size_t i = 0; i < SUPPORTED_VERSIONS.size(); ++i) {
        if (SUPPORTED_VERSIONS[i] == version) {
            return i;
        }
    }
    return SUPPORTED_VERSIONS.size();
}

/** The supported version a listed version is, or is routed as. */
constexpr std::optional<int> getKnownProtocolVersion(int protocol_version)
{
    for (const int version : SUPPORTED_VERSIONS) {
        if (version == protocol_version) {
            return version;
        }
    }
    for (const auto &[announced, routes_as] : WIRE_IDENTICAL) {
        if (announced == protocol_version) {
            return routes_as;
        }
    }
    return std::nullopt;
}

/** The supported version to route a version as. One more than a listed version is routed as that one
 * too: Bedrock adds one for a release after its last preview, or for a hotfix, and the packets have
 * mostly stayed the same, but no schema has checked it.
 * @see Velocity ProtocolVersion#getProtocolVersion(int). */
constexpr std::optional<int> getProtocolVersion(int protocol_version)
{
    if (const auto known = getKnownProtocolVersion(protocol_version)) {
        return known;
    }
    return getKnownProtocolVersion(protocol_version - 1);
}

template <class F>
constexpr bool visit(int version, F &&visitor)
{
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
        return (
            (SUPPORTED_VERSIONS[I] == version ? (visitor.template operator()<SUPPORTED_VERSIONS[I]>(), true) : false) ||
            ...);
    }(std::make_index_sequence<SUPPORTED_VERSIONS.size()>{});
}

} // namespace ProtocolVersions

consteval int step(int from, int to)
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
