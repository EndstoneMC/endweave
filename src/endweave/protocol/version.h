#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <utility>

namespace endweave {

namespace ProtocolVersions {

/** @see Velocity ProtocolVersion#SUPPORTED_VERSIONS. */
constexpr std::array SUPPORTED_VERSIONS{
    2168, // 1.26.40
    2192, // 1.26.50
};

/** Versions that encode every packet the same as a supported version and are routed as it.
 * `identical.h` checks each entry at compile time. */
constexpr std::array<std::pair<int, int>, 1> WIRE_IDENTICAL{{
    {2169, 2168}, // 1.26.45
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

/** @see Velocity ProtocolVersion#getProtocolVersion(int). */
constexpr std::optional<int> getProtocolVersion(int protocol_version)
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
