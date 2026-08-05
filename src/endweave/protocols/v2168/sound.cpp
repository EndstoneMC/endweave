#include "endweave/protocols/v2168/sound.h"

#include <utility>

namespace endweave {

bp::PlaySoundPacket_<1001> Transformer<bp::PlaySoundPacket_<2168>, bp::PlaySoundPacket_<1001>>::transform(
    bp::PlaySoundPacket_<2168> &&from)
{
    bp::PlaySoundPacket_<1001> to;
    to.name = std::move(from.name);
    to.pos = from.pos;
    to.volume = from.volume;
    to.pitch = from.pitch;
    // ENDWEAVE: loop_count is dropped; a 1001 client plays the sound once, and the handle still lets the
    // server stop it.
    to.server_sound_handle = from.server_sound_handle;
    return to;
}

bp::LevelSoundEventPacket_<1001> Transformer<bp::LevelSoundEventPacket_<2168>, bp::LevelSoundEventPacket_<1001>>::
    transform(bp::LevelSoundEventPacket_<2168> &&from)
{
    bp::LevelSoundEventPacket_<1001> to;
    // ENDWEAVE: TODO the name passes through, but 2168's Mount, Dismount and StrawBedBreakLeave resolve to
    // nothing at 1001; substituting an old sound would need a hand-kept table and still be a guess.
    to.sound_event = std::move(from.sound_event);
    to.pos = from.pos;
    to.data = from.data;
    to.actor_identifier = std::move(from.actor_identifier);
    to.is_baby = from.is_baby;
    to.is_global = from.is_global;
    to.actor = from.actor;
    to.fire_at_position = from.fire_at_position;
    return to;
}

bp::ClientboundUpdateSoundDataPacket_<1001> Transformer<
    bp::ClientboundUpdateSoundDataPacket_<2168>,
    bp::ClientboundUpdateSoundDataPacket_<1001>>::transform(bp::ClientboundUpdateSoundDataPacket_<2168> &&from)
{
    bp::ClientboundUpdateSoundDataPacket_<1001> to;
    to.server_sound_handle = from.server_sound_handle;
    // ENDWEAVE: TODO 1001's SoundDataEvent has only Stop, so SetVolume, SetPitch, Fade, SeekTo, Pause and
    // Resume all end the sound instead of adjusting it. Refuse those six once there is an error channel.
    to.sound_event = bp::SoundDataEvent_<1001>::STOP;
    return to;
}

} // namespace endweave
