#pragma once

#include "endweave/protocol/version.h"

namespace endweave {

/** Semantic fixups that the wire diff cannot see, keyed on the two ends of the connection
 * and applied once to the source packet before the chain runs.
 *
 * Keyed on the pair rather than a single version because the work is a translation: a
 * value numbered in `From` has to come out numbered in `To`, and no single version knows
 * both. Applied once rather than per hop because a name that `From` and `To` share may be
 * missing from a version between them -- remapping at every hop would fall back to that
 * version's sentinel and lose it, where one lookup from `From` straight to `To` does not.
 *
 * It writes `To`'s value into the `From` struct, which is what lets it sit beside a
 * `WireCompatible` pair: the fix is already made for `To`, so those bytes need no
 * destination struct and the packet is re-encoded at `From`. Everything between must carry
 * the field across untouched, which is what a transform does with a field it does not
 * reshape.
 *
 * Every version bump renumbers something in this class, so the specializations are a
 * standing checklist rather than a one-off per pair. */
template <ProtocolVersion From, ProtocolVersion To, int Id>
struct Rewriter;

} // namespace endweave
