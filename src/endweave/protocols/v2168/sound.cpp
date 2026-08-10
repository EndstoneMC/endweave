#include "endweave/protocols/v2168/sound.h"

#include <bedrock/enum.hpp>
#include <expected>
#include <system_error>
#include <utility>
#include <variant>

namespace endweave {

bp::LevelSoundEvent_<1001> Transformer<bp::LevelSoundEvent_<2168>, bp::LevelSoundEvent_<1001>>::transform(
    bp::LevelSoundEvent_<2168> &&from)
{
    using To = bp::LevelSoundEvent_<1001>;
    return bp::enum_cast<To>(bp::enum_name(from)).value_or(To::UNDEFINED);
}

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

std::expected<bp::ClientboundUpdateSoundDataPacket_<1001>, std::error_code> Transformer<
    bp::ClientboundUpdateSoundDataPacket_<2168>,
    bp::ClientboundUpdateSoundDataPacket_<1001>>::transform(bp::ClientboundUpdateSoundDataPacket_<2168> &&from)
{
    // ENDWEAVE: Stop is all 1001's SoundDataEvent has. SetVolume, SetPitch, Fade, SeekTo, Pause and
    // Resume adjust a playing sound, and stopping it instead is further from what the server meant
    // than saying nothing.
    if (!std::holds_alternative<bp::Stop_<2168>>(from.event)) {
        return std::unexpected(std::make_error_code(std::errc::not_supported));
    }
    bp::ClientboundUpdateSoundDataPacket_<1001> to;
    to.server_sound_handle = from.server_sound_handle;
    to.sound_event = bp::SoundDataEvent_<1001>::STOP;
    return to;
}

} // namespace endweave
