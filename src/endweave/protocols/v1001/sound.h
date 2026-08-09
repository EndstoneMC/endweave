#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/sound.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlaySoundPacket_<1001>, bp::PlaySoundPacket_<2168>> {
    static bp::PlaySoundPacket_<2168> transform(bp::PlaySoundPacket_<1001> &&from);
};

// ENDWEAVE: the event is a name at both versions, so 2168's added enumerators and its moved Undefined
// sentinel never reach the wire; every 1001 name is still a 2168 name.
template <>
struct WireCompatible<bp::LevelSoundEventPacket_<1001>, bp::LevelSoundEventPacket_<2168>> : std::true_type {};

template <>
struct Transformer<bp::ClientboundUpdateSoundDataPacket_<1001>, bp::ClientboundUpdateSoundDataPacket_<2168>> {
    static bp::ClientboundUpdateSoundDataPacket_<2168> transform(bp::ClientboundUpdateSoundDataPacket_<1001> &&from);
};

} // namespace endweave
