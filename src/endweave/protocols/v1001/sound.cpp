#include "endweave/protocols/v1001/sound.h"

#include <bedrock/enum.hpp>
#include <utility>

namespace endweave {

void Transformer<bp::LevelSoundEvent_<1001>, bp::LevelSoundEvent_<2168>>::transform(
    Context<bp::LevelSoundEvent_<2168>> &ctx, bp::LevelSoundEvent_<1001> &&from)
{
    using To = bp::LevelSoundEvent_<2168>;
    ctx.out() = bp::enum_cast<To>(bp::enum_name(from)).value_or(To::UNDEFINED);
}

void Transformer<bp::PlaySoundPacket_<1001>, bp::PlaySoundPacket_<2168>>::transform(
    Context<bp::PlaySoundPacket_<2168>> &ctx, bp::PlaySoundPacket_<1001> &&from)
{
    auto &to = ctx.out();
    to.name = std::move(from.name);
    to.pos = from.pos;
    to.volume = from.volume;
    to.pitch = from.pitch;
    // ENDWEAVE: TODO 1001 has no loop count; 0 is play-once, since BDS passes it to FMOD where the count is
    // repeats after the first play. Inferred from the audio layer, not confirmed against BDS.
    to.loop_count = 0;
    to.server_sound_handle = from.server_sound_handle;
}

void Transformer<bp::ClientboundUpdateSoundDataPacket_<1001>, bp::ClientboundUpdateSoundDataPacket_<2168>>::transform(
    Context<bp::ClientboundUpdateSoundDataPacket_<2168>> &ctx, bp::ClientboundUpdateSoundDataPacket_<1001> &&from)
{
    auto &to = ctx.out();
    to.server_sound_handle = from.server_sound_handle;
    // ENDWEAVE: Stop is the only value 1001's SoundDataEvent has, so it is the only one of 2168's seven
    // cases a 1001 packet can select; the other six only ever come the other way.
    to.event = bp::Stop_<2168>{};
}

} // namespace endweave
