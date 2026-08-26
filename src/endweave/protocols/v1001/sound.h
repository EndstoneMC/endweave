#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/sound.h>

namespace bp = bedrock::protocol;

namespace endweave {

// ENDWEAVE: new sounds are appended before the Undefined sentinel, so its number moves every
// version -- 611 at 1001, 614 at 2168 -- and a number passed through lands on whatever sound
// took its place. Matching on the generated name instead is exact, and the sentinel comes out
// as the sentinel.
template <>
struct Transformer<bp::LevelSoundEvent_<1001>, bp::LevelSoundEvent_<2168>> {
    static void transform(Context<bp::LevelSoundEvent_<2168>> &ctx, bp::LevelSoundEvent_<1001> &&from);
};

template <>
struct Transformer<bp::PlaySoundPacket_<1001>, bp::PlaySoundPacket_<2168>> {
    static void transform(Context<bp::PlaySoundPacket_<2168>> &ctx, bp::PlaySoundPacket_<1001> &&from);
};

// ENDWEAVE: the event is a name at both versions, so 2168's added enumerators and its moved Undefined
// sentinel never reach the wire; every 1001 name is still a 2168 name.
template <>
struct WireCompatible<bp::LevelSoundEventPacket_<1001>, bp::LevelSoundEventPacket_<2168>> : std::true_type {};

template <>
struct Transformer<bp::ClientboundUpdateSoundDataPacket_<1001>, bp::ClientboundUpdateSoundDataPacket_<2168>> {
    static void transform(Context<bp::ClientboundUpdateSoundDataPacket_<2168>> &ctx,
                          bp::ClientboundUpdateSoundDataPacket_<1001> &&from);
};

} // namespace endweave
