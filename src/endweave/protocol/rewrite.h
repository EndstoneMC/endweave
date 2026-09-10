#pragma once

#include "endweave/protocol/packet.h"

#include <concepts>

namespace endweave {

/** Fixes values the wire diff can't see, such as renumbered enums. Runs once on the source
 * packet, mapping straight from `From` to `To`, since a value both share may be missing from a
 * version in between.
 *
 * It writes `To`'s values into the `From` packet, so a packet whose type doesn't change can be
 * re-encoded as is. */
template <int From, int To, int Id>
struct Rewriter;

template <int From, int To, int Id>
concept Rewritable = requires(packet_of_t<From, Id> &packet) {
    { Rewriter<From, To, Id>::rewrite(packet) } -> std::same_as<void>;
};

} // namespace endweave
