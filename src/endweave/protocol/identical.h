#pragma once

#include "endweave/protocol/version.h"

// The umbrella, not a module: `packet_of` answers `void` for an id whose header was not stamped in,
// and two `void`s compare equal, so a lone module include would make this test pass vacuously.
#include <bedrock/protocol.hpp>
#include <bedrock/protocol/packet.hpp>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace bp = bedrock::protocol;

namespace endweave {

namespace detail {

/** The id space the wire can carry: `PacketHeader` packs the id in ten bits. Sweeping all of it
 * rather than one version's `EndId` is what catches an id that only one of the two models. */
inline constexpr std::size_t kIdSpace = 1024;

} // namespace detail

/** Whether two protocol versions encode every packet the schema models identically. Type identity
 * is the wire diff -- bedrock-protocol emits one type per distinct shape -- so a pair that agrees
 * at every id needs no transform between them and can share one set of handler tables. */
template <int A, int B>
consteval bool wireIdentical()
{
    return []<std::size_t... I>(std::index_sequence<I...>) {
        return (std::is_same_v<bp::packet_of_t<A, I>, bp::packet_of_t<B, I>> && ...);
    }(std::make_index_sequence<detail::kIdSpace>{});
}

namespace detail {

template <std::size_t I>
consteval bool aliasHoldsAgainstSchema()
{
    constexpr auto entry = ProtocolVersions::WIRE_IDENTICAL[I];
    static_assert(wireIdentical<entry.first, static_cast<int>(entry.second)>(),
                  "endweave: a WIRE_IDENTICAL entry no longer matches the schema. The version on the left is "
                  "routed as the one on the right on the strength of encoding every packet the same way, and "
                  "it now does not. Give it its own SUPPORTED_VERSIONS entry and write the transforms for the "
                  "packets that differ, or drop the entry if the version is gone.");
    return true;
}

template <std::size_t... I>
consteval bool aliasesHoldAgainstSchema(std::index_sequence<I...>)
{
    return (aliasHoldsAgainstSchema<I>() && ...);
}

} // namespace detail

// Every entry is checked, so adding one to the table cannot skip this.
static_assert(detail::aliasesHoldAgainstSchema(std::make_index_sequence<ProtocolVersions::WIRE_IDENTICAL.size()>{}));

} // namespace endweave
