#include "sound.h"

#include <bedrock/protocol/enum.hpp>
#include <expected>
#include <optional>
#include <system_error>
#include <utility>
#include <variant>

namespace endweave {
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
