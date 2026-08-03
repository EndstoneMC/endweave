#pragma once

#include <array>

namespace endweave {

/**
 * The Bedrock protocol versions endweave has a node for. The enumerator name is the game version
 * in ViaVersion's vX_Y style, and the value is the on-wire protocol number. bedrock-protocol's
 * generated _<V> selectors key on raw int, so endweave owns this naming and passes
 * static_cast<int>(V) across the boundary.
 *
 * @see ViaVersion ProtocolVersion, whose recent constants are likewise named v26_1, v26_2, ...
 */
enum class ProtocolVersion : int {
    v26_20 = 975,
    v26_30 = 1001,
};

/**
 * The version chain, ascending. ProtocolManager::registerProtocols registers in this order, and a
 * node reads its predecessor off it to know which diff it owns.
 *
 * @note endweave-specific. ViaVersion reads an edge's two ends off the protocol's own generic
 * parameters, which a node has nowhere to put.
 */
inline constexpr std::array kProtocolVersions{
    ProtocolVersion::v26_20,
    ProtocolVersion::v26_30,
};

/**
 * @param version The version to look up.
 * @return true if the chain has a node for it.
 * @note endweave-specific, alongside kProtocolVersions.
 */
constexpr bool isModelled(ProtocolVersion version)
{
    for (const ProtocolVersion known : kProtocolVersions) {
        if (known == version) {
            return true;
        }
    }
    return false;
}

/**
 * @param version The version to look up.
 * @return The version before it in the chain. The root is its own predecessor, so the diff a root
 * node owns is empty.
 * @note endweave-specific, alongside kProtocolVersions.
 */
constexpr ProtocolVersion previousOf(ProtocolVersion version)
{
    ProtocolVersion previous = version;
    for (const ProtocolVersion known : kProtocolVersions) {
        if (known == version) {
            break;
        }
        previous = known;
    }
    return previous;
}

} // namespace endweave
