#pragma once

#include <cstddef>

namespace endweave {

/**
 * Transport direction of a packet. Serverbound is client to server, clientbound the reverse.
 *
 * @see ViaVersion Direction.
 */
enum class Direction {
    Serverbound,
    Clientbound
};

/**
 * Version step of a single hop. A version node picks its table by this axis instead of the
 * transport direction, since the same edge is walked both ways across connections.
 *
 * @note endweave-specific: ViaVersion protocols are single-direction edges and have no Step.
 */
enum class Step {
    Upgrade,
    Downgrade
};

/**
 * Maps a transport direction onto one of a protocol's two handler tables.
 *
 * @see ViaVersion AbstractProtocol#transform, which selects the table inline by direction.
 */
constexpr std::size_t slotOf(Direction direction)
{
    return direction == Direction::Serverbound ? 0 : 1;
}

/** Maps a version step onto one of a protocol's two handler tables. */
constexpr std::size_t slotOf(Step step)
{
    return step == Step::Upgrade ? 0 : 1;
}

/** Reverses a version step, for walking a protocol path backwards. */
constexpr Step invert(Step step)
{
    return step == Step::Upgrade ? Step::Downgrade : Step::Upgrade;
}

} // namespace endweave
