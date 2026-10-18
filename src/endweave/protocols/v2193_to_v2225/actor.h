#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/actor.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::AddActorPacket_<2193>, bp::AddActorPacket_<2225>> {
    static void transform(Context<bp::AddActorPacket_<2225>> &ctx, bp::AddActorPacket_<2193> &&from);
};

template <>
struct Transformer<bp::AnimatePacket_<2193>, bp::AnimatePacket_<2225>> {
    static void transform(Context<bp::AnimatePacket_<2225>> &ctx, bp::AnimatePacket_<2193> &&from);
};
} // namespace endweave
