#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/sound.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlaySoundPacket_<2168>, bp::PlaySoundPacket_<1001>> {
    static bp::PlaySoundPacket_<1001> transform(bp::PlaySoundPacket_<2168> &&from);
};

template <>
struct Transformer<bp::LevelSoundEventPacket_<2168>, bp::LevelSoundEventPacket_<1001>> {
    static bp::LevelSoundEventPacket_<1001> transform(bp::LevelSoundEventPacket_<2168> &&from);
};

template <>
struct Transformer<bp::ClientboundUpdateSoundDataPacket_<2168>, bp::ClientboundUpdateSoundDataPacket_<1001>> {
    static bp::ClientboundUpdateSoundDataPacket_<1001> transform(bp::ClientboundUpdateSoundDataPacket_<2168> &&from);
};

} // namespace endweave
