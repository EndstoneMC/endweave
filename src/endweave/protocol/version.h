#pragma once

#include <array>
#include <cstddef>
#include <utility>

namespace endweave {

/** @see ViaVersion ProtocolVersion. */
enum class ProtocolVersion : int {
    UNKNOWN = -1,
    v26_20 = 975,
    v26_30 = 1001,
    v26_40 = 2168,
};

namespace ProtocolVersions {

/** @see Velocity ProtocolVersion#SUPPORTED_VERSIONS. */
constexpr std::array SUPPORTED_VERSIONS{
    ProtocolVersion::v26_30,
    ProtocolVersion::v26_40,
};

/** @see Velocity ProtocolVersion#getProtocolVersion(int). */
constexpr ProtocolVersion getProtocolVersion(int protocol_version)
{
    for (const ProtocolVersion version : SUPPORTED_VERSIONS) {
        if (static_cast<int>(version) == protocol_version) {
            return version;
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

consteval ProtocolVersion next(ProtocolVersion version)
{
    for (std::size_t i = 0; i + 1 < ProtocolVersions::SUPPORTED_VERSIONS.size(); ++i) {
        if (ProtocolVersions::SUPPORTED_VERSIONS[i] == version) {
            return ProtocolVersions::SUPPORTED_VERSIONS[i + 1];
        }
    }
    return version;
}

consteval ProtocolVersion prev(ProtocolVersion version)
{
    for (std::size_t i = 1; i < ProtocolVersions::SUPPORTED_VERSIONS.size(); ++i) {
        if (ProtocolVersions::SUPPORTED_VERSIONS[i] == version) {
            return ProtocolVersions::SUPPORTED_VERSIONS[i - 1];
        }
    }
    return version;
}

} // namespace endweave
