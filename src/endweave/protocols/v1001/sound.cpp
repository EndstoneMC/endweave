#include "endweave/protocols/v1001/sound.h"

#include <utility>

namespace endweave {

bp::PlaySoundPacket_<2168> Transformer<bp::PlaySoundPacket_<1001>, bp::PlaySoundPacket_<2168>>::transform(
    bp::PlaySoundPacket_<1001> &&from)
{
    bp::PlaySoundPacket_<2168> to;
    to.name = std::move(from.name);
    to.pos = from.pos;
    to.volume = from.volume;
    to.pitch = from.pitch;
    // ENDWEAVE: TODO 1001 has no loop count; 0 is play-once, since BDS passes it to FMOD where the count is
    // repeats after the first play. Inferred from the audio layer, not confirmed against BDS.
    to.loop_count = 0;
    to.server_sound_handle = from.server_sound_handle;
    return to;
}

bp::ClientboundUpdateSoundDataPacket_<2168> Transformer<
    bp::ClientboundUpdateSoundDataPacket_<1001>,
    bp::ClientboundUpdateSoundDataPacket_<2168>>::transform(bp::ClientboundUpdateSoundDataPacket_<1001> &&from)
{
    bp::ClientboundUpdateSoundDataPacket_<2168> to;
    to.server_sound_handle = from.server_sound_handle;
    // ENDWEAVE: Stop is the only value 1001's SoundDataEvent has, so it is the only one of 2168's seven
    // cases a 1001 packet can select; the other six only ever come the other way.
    to.sound_event = bp::Stop_<2168>{};
    return to;
}

} // namespace endweave
