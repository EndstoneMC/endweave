#pragma once

#include <bedrock/protocol/packet.hpp>
#include <concepts>
#include <expected>
#include <system_error>

namespace bp = bedrock::protocol;

namespace endweave {

/** Fixes values the wire diff can't see, such as renumbered enums. Runs once on the source
 * packet, mapping straight from `From` to `To`, since a value both share may be missing from a
 * version in between.
 *
 * It writes `To`'s values into the `From` packet, so a packet whose type doesn't change can be
 * re-encoded as is. One that parses bytes the schema leaves opaque returns an error for bytes it
 * can't read. */
template <int From, int To, int Id>
struct Rewriter;

template <int From, int To, int Id>
concept Rewritable = requires(bp::packet_of_t<From, Id> &packet) {
    requires std::same_as<decltype(Rewriter<From, To, Id>::rewrite(packet)), void> ||
                 std::same_as<decltype(Rewriter<From, To, Id>::rewrite(packet)), std::expected<void, std::error_code>>;
};

} // namespace endweave
