#pragma once

#include <bedrock/protocol/packet.hpp>
#include <concepts>
#include <expected>
#include <system_error>
#include <tuple>

namespace bp = bedrock::protocol;

namespace endweave {

class BlockConnectionStorage;
class ItemRegistryStorage;
class DimensionDataStorage;

/** The state a connection's translation keeps across packets, which `handle` hands a rewriter by the
 * types its `rewrite` takes.
 * @see ViaVersion UserConnection#get. */
using Storages = std::tuple<BlockConnectionStorage &, ItemRegistryStorage &, DimensionDataStorage &>;

/** Fixes values the wire diff can't see, such as renumbered enums. Runs once on the source
 * packet, mapping straight from `From` to `To`, since a value both share may be missing from a
 * version in between.
 *
 * It writes `To`'s values into the `From` packet, so a packet whose type doesn't change can be
 * re-encoded as is. One that parses bytes the schema leaves opaque returns an error for bytes it
 * can't read. One that keeps state across packets takes the storages it needs after the packet, any
 * of `Storages`. One that gives the packet's block ids itself sets `rewrites_block_ids`. */
template <int From, int To, int Id>
struct Rewriter;

template <class T>
concept RewriteResult = std::same_as<T, void> || std::same_as<T, std::expected<void, std::error_code>>;

template <int From, int To, int Id>
concept Rewritable = requires { &Rewriter<From, To, Id>::rewrite; };

template <int From, int To, int Id>
concept RewritesBlockIds = Rewritable<From, To, Id> && Rewriter<From, To, Id>::rewrites_block_ids;

} // namespace endweave
