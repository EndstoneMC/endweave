#pragma once

#include "endweave/protocol/version.h"

#include <type_traits>

namespace endweave {

/** @see ViaVersion Protocol#cancelServerbound, Protocol#cancelClientbound. */
template <ProtocolVersion From, ProtocolVersion To, int Id>
struct Cancel : std::false_type {};

template <ProtocolVersion From, ProtocolVersion To, int Id>
inline constexpr bool cancel_v = Cancel<From, To, Id>::value;

} // namespace endweave
