#pragma once

#include <cstddef>

namespace endweave {

/**
 * Transport direction of a packet, matching ViaVersion's Direction.
 *
 * Serverbound is client to server, clientbound is server to client. A base protocol picks its
 * handler table by this axis.
 */
enum class Direction {
    Serverbound,
    Clientbound
};

/**
 * Version step of a single hop along a protocol path.
 *
 * A version protocol picks its handler table by this axis instead of the transport direction:
 * endweave bridges older-client and newer-client alike, so a clientbound packet may need
 * either an upgrade or a downgrade depending on how the server and client versions compare.
 */
enum class Step {
    Upgrade,
    Downgrade
};

/**
 * Maps a transport direction onto one of a protocol's two handler tables.
 *
 * @param direction The transport direction.
 * @return The table index.
 */
constexpr std::size_t slotOf(Direction direction)
{
    return direction == Direction::Serverbound ? 0 : 1;
}

/**
 * Maps a version step onto one of a protocol's two handler tables.
 *
 * @param step The version step.
 * @return The table index.
 */
constexpr std::size_t slotOf(Step step)
{
    return step == Step::Upgrade ? 0 : 1;
}

/**
 * Reverses a version step, for walking a protocol path backwards.
 *
 * @param step The version step.
 * @return The opposite step.
 */
constexpr Step invert(Step step)
{
    return step == Step::Upgrade ? Step::Downgrade : Step::Upgrade;
}

} // namespace endweave
