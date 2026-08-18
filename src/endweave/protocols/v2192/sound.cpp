#include "endweave/protocols/v2192/sound.h"

#include <utility>

namespace endweave {

void Transformer<bp::PlaySoundPacket_<2192>, bp::PlaySoundPacket_<2168>>::transform(
    Context<bp::PlaySoundPacket_<2168>> &ctx, bp::PlaySoundPacket_<2192> &&from)
{
    auto &to = ctx.out();
    to.name = std::move(from.name);
    to.pos = from.pos;
    to.volume = from.volume;
    to.pitch = from.pitch;
    to.loop_count = from.loop_count;
    // ENDWEAVE: bypass_listener_range_check is dropped; a 2168 client hears the sound only if it is in
    // range, so a sound meant to carry regardless goes unheard rather than arriving misplaced.
    to.server_sound_handle = from.server_sound_handle;
    // ENDWEAVE: playback_position_seconds is dropped; a 2168 client starts the sound from the beginning
    // rather than partway in.
}

} // namespace endweave
