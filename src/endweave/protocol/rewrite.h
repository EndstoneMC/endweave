#pragma once

#include <bedrock/protocol/packet.hpp>
#include <concepts>
#include <expected>
#include <system_error>

namespace bp = bedrock::protocol;

namespace endweave {

class BlockConnectionStorage;
class ItemRegistryStorage;

/** Fixes values the wire diff can't see, such as renumbered enums. Runs once on the source
 * packet, mapping straight from `From` to `To`, since a value both share may be missing from a
 * version in between.
 *
 * It writes `To`'s values into the `From` packet, so a packet whose type doesn't change can be
 * re-encoded as is. One that parses bytes the schema leaves opaque returns an error for bytes it
 * can't read. One that needs the blocks the client was sent before takes the
 * `BlockConnectionStorage` as well, and one that needs the items the client lacks the
 * `ItemRegistryStorage`. One that gives the packet's block ids itself sets `rewrites_block_ids`. */
template <int From, int To, int Id>
struct Rewriter;

template <class T>
concept RewriteResult = std::same_as<T, void> || std::same_as<T, std::expected<void, std::error_code>>;

template <int From, int To, int Id, class... Storage>
concept RewritesWith = requires(bp::packet_of_t<From, Id> &packet, Storage &...storage) {
    requires RewriteResult<decltype(Rewriter<From, To, Id>::rewrite(packet, storage...))>;
};

template <int From, int To, int Id>
concept Rewritable = RewritesWith<From, To, Id> || RewritesWith<From, To, Id, BlockConnectionStorage> ||
                     RewritesWith<From, To, Id, ItemRegistryStorage>;

template <int From, int To, int Id>
concept RewritesBlockIds = Rewritable<From, To, Id> && Rewriter<From, To, Id>::rewrites_block_ids;

} // namespace endweave
