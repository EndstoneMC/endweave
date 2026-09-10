#pragma once

#include "endweave/protocol/packet.h"
#include "endweave/protocol/version.h"

#include <cstddef>
#include <type_traits>
#include <utility>

namespace endweave {

namespace detail {

/** Every id `PacketHeader`'s 10-bit field can hold, so ids that only one version defines are compared too. */
inline constexpr std::size_t kIdSpace = 1024;

} // namespace detail

/** Whether two protocol versions use the same packet type at every id. bedrock-protocol emits one
 * type per wire shape, so this means they encode identically. */
template <int A, int B>
inline constexpr bool wire_identical_v = []<std::size_t... I>(std::index_sequence<I...>) {
    return (std::is_same_v<bp::packet_of_t<A, I>, bp::packet_of_t<B, I>> && ...);
}(std::make_index_sequence<detail::kIdSpace>{});

namespace detail {

template <std::size_t I>
consteval bool aliasHoldsAgainstSchema()
{
    constexpr auto entry = ProtocolVersions::WIRE_IDENTICAL[I];
    static_assert(wire_identical_v<entry.first, entry.second>,
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

// Checks every WIRE_IDENTICAL entry.
static_assert(detail::aliasesHoldAgainstSchema(std::make_index_sequence<ProtocolVersions::WIRE_IDENTICAL.size()>{}));

} // namespace endweave
