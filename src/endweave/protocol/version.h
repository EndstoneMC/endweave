#pragma once

#include <array>
#include <cstddef>
#include <string_view>
#include <utility>

namespace endweave {

/** @see ViaVersion ProtocolVersion. */
enum class ProtocolVersion : int {
    UNKNOWN = -1,
    v26_20 = 975,
    v26_30 = 1001,
    v26_40 = 2168,
    /** 1.26.44 reshaped SetScorePacket but shipped under 2168, so the dialect earns an id of
     * endweave's own. Mojang closed the gap to 2192 with 2177, 2181, 2187 and 2192, so 2169 stays
     * free. It never reaches the wire -- `networkVersion` maps it back to the 2168 it announces as. */
    v26_44 = 2169,
    v26_50 = 2192,
};

namespace ProtocolVersions {

/** @see Velocity ProtocolVersion#SUPPORTED_VERSIONS. */
constexpr std::array SUPPORTED_VERSIONS{
    ProtocolVersion::v26_30,
    ProtocolVersion::v26_40,
    ProtocolVersion::v26_44,
    ProtocolVersion::v26_50,
};

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
    return ProtocolVersion::UNKNOWN;
}

/** The number a dialect announces. `v26_44` names a version Mojang never numbered, so it has to
 * go on the wire as the 2168 every 1.26.4x build reports. */
constexpr int networkVersion(ProtocolVersion version)
{
    return static_cast<int>(version == ProtocolVersion::v26_44 ? ProtocolVersion::v26_40 : version);
}

/** Whether a dotted version is at or past `minor.patch`. Two spellings reach this and they do
 * not agree on how many components they carry: Endstone spells the server's version
 * `26.40` and a client's `1.26.50`, so a leading `1` is Bedrock's major and is dropped where it
 * appears. A string that does not parse reads as older, which leaves an unrecognised build on the
 * shape every 2168 client understood. */
constexpr bool atLeast(std::string_view version, int minor, int patch)
{
    int parts[3] = {0, 0, 0};
    std::size_t count = 0;
    std::size_t at = 0;
    while (at < version.size() && count < 3 && version[at] >= '0' && version[at] <= '9') {
        int have = 0;
        while (at < version.size() && version[at] >= '0' && version[at] <= '9') {
            have = have * 10 + (version[at] - '0');
            ++at;
        }
        parts[count++] = have;
        if (at < version.size() && version[at] == '.') {
            ++at;
        }
    }
    const bool majored = count > 0 && parts[0] == 1;
    const int *v = majored ? parts + 1 : parts;
    if ((majored ? count - 1 : count) < 2) {
        return false;
    }
    return v[0] != minor ? v[0] > minor : v[1] >= patch;
}

/** 1.26.40 and 1.26.44 announce the same 2168 and disagree on SetScorePacket, so the version is
 * the only thing that tells the two dialects apart. */
constexpr ProtocolVersion dialectOf(ProtocolVersion announced, std::string_view version)
{
    if (announced == ProtocolVersion::v26_40 && atLeast(version, 26, 44)) {
        return ProtocolVersion::v26_44;
    }
    return announced;
}

// Both spellings, and the boundary either side of it.
static_assert(!atLeast("26.40", 26, 44));
static_assert(!atLeast("26.43", 26, 44));
static_assert(atLeast("26.44", 26, 44));
static_assert(atLeast("26.50", 26, 44));
static_assert(!atLeast("1.26.40", 26, 44));
static_assert(!atLeast("1.26.43.1", 26, 44));
static_assert(atLeast("1.26.44", 26, 44));
static_assert(atLeast("1.26.50.26", 26, 44));
static_assert(atLeast("1.27.0", 26, 44));
static_assert(!atLeast("", 26, 44));
static_assert(!atLeast("unknown", 26, 44));
static_assert(dialectOf(ProtocolVersion::v26_40, "26.40") == ProtocolVersion::v26_40);
static_assert(dialectOf(ProtocolVersion::v26_40, "1.26.44") == ProtocolVersion::v26_44);
static_assert(dialectOf(ProtocolVersion::v26_50, "1.26.50.26") == ProtocolVersion::v26_50);

/** The version a clientbound packet arrives as. Endstone already rewrites SetScorePacket to
 * 1.26.43's shape for every client in [1.26.40, 1.26.44) -- exactly those that resolve to
 * `v26_40` -- so those connections carry that dialect's bytes whatever the server speaks.
 * @see Endstone downgradeSetScorePayload, whose TODO(1.26.50) retires this alongside it. */
constexpr ProtocolVersion clientboundSource(ProtocolVersion client_version, ProtocolVersion server_version)
{
    return client_version == ProtocolVersion::v26_40 ? ProtocolVersion::v26_40 : server_version;
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
