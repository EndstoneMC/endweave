#include "endweave/protocols/v2168/sound.h"

#include <bedrock/protocol/enum.hpp>
#include <expected>
#include <optional>
#include <system_error>
#include <utility>
#include <variant>

namespace endweave {

void Transformer<bp::LevelSoundEvent_<2168>, bp::LevelSoundEvent_<1001>>::transform(
    Context<bp::LevelSoundEvent_<1001>> &ctx, bp::LevelSoundEvent_<2168> &&from)
{
    using To = bp::LevelSoundEvent_<1001>;
    ctx.out() = bp::enum_cast<To>(bp::enum_name(from)).value_or(To::UNDEFINED);
}

void Transformer<bp::ClientboundUpdateSoundDataPacket_<2168>, bp::ClientboundUpdateSoundDataPacket_<1001>>::transform(
    Context<bp::ClientboundUpdateSoundDataPacket_<1001>> &ctx, bp::ClientboundUpdateSoundDataPacket_<2168> &&from)
{
    // ENDWEAVE: Stop is all 1001's SoundDataEvent has. SetVolume, SetPitch, Fade, SeekTo, Pause and
    // Resume adjust a playing sound, and stopping it instead is further from what the server meant
    // than saying nothing.
    if (!std::holds_alternative<bp::Stop_<2168>>(from.event)) {
        ctx.cancel();
        return;
    }
    auto &to = ctx.out();
    to.server_sound_handle = from.server_sound_handle;
    to.sound_event = bp::SoundDataEvent_<1001>::STOP;
}

void Transformer<bp::PlaySoundPacket_<2168>, bp::PlaySoundPacket_<2192>>::transform(
    Context<bp::PlaySoundPacket_<2192>> &ctx, bp::PlaySoundPacket_<2168> &&from)
{
    auto &to = ctx.out();
    to.name = std::move(from.name);
    to.pos = from.pos;
    to.volume = from.volume;
    to.pitch = from.pitch;
    to.loop_count = from.loop_count;
    // ENDWEAVE: 2168 has nothing to say about the listener range check, and false is the check BDS
    // applied before the flag existed.
    to.bypass_listener_range_check = false;
    to.server_sound_handle = from.server_sound_handle;
    // ENDWEAVE: 2168 never names a playback position, so the sound starts at the beginning.
    to.playback_position_seconds = std::nullopt;
}

} // namespace endweave
