#pragma once

#include <bedrock/protocol/serializer.hpp>

namespace bp = bedrock::protocol;

namespace endweave {

/** Whether protocols `A` and `B` encode `T` through one `Serializer`. */
template <class T, int A, int B>
    requires bp::Serializable<T, A> && bp::Serializable<T, B>
inline constexpr bool same_encoding_v = &bp::Serializer<T, A>::serialize == &bp::Serializer<T, B>::serialize &&
                                      &bp::Serializer<T, A>::deserialize == &bp::Serializer<T, B>::deserialize;

} // namespace endweave
