#pragma once

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

} // namespace endweave
