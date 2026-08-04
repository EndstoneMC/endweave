#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/sound.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlaySoundPacket_<2168>> {
    static bp::PlaySoundPacket_<1001> downgrade(bp::PlaySoundPacket_<2168> &&from);
};

template <>
struct Transformer<bp::LevelSoundEventPacket_<2168>> {
    static bp::LevelSoundEventPacket_<1001> downgrade(bp::LevelSoundEventPacket_<2168> &&from);
};

template <>
struct Transformer<bp::ClientboundUpdateSoundDataPacket_<2168>> {
    static bp::ClientboundUpdateSoundDataPacket_<1001> downgrade(bp::ClientboundUpdateSoundDataPacket_<2168> &&from);
};

} // namespace endweave
