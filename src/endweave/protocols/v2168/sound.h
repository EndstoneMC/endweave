#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/sound.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlaySoundPacket_<2168>, bp::PlaySoundPacket_<1001>> {
    static bp::PlaySoundPacket_<1001> transform(bp::PlaySoundPacket_<2168> &&from);
};

// ENDWEAVE: TODO the name passes through, but 2168's Mount, Dismount and StrawBedBreakLeave resolve to
// nothing at 1001; substituting an old sound would need a hand-kept table and still be a guess.
template <>
struct WireCompatible<bp::LevelSoundEventPacket_<2168>, bp::LevelSoundEventPacket_<1001>> : std::true_type {};

template <>
struct Transformer<bp::ClientboundUpdateSoundDataPacket_<2168>, bp::ClientboundUpdateSoundDataPacket_<1001>> {
    static bp::ClientboundUpdateSoundDataPacket_<1001> transform(bp::ClientboundUpdateSoundDataPacket_<2168> &&from);
};

} // namespace endweave
