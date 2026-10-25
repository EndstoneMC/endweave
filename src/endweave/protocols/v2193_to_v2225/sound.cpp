#include "sound.h"

#include <utility>

namespace ew = endweave;

namespace endweave {
void Transformer<bp::ClientboundUpdateSoundDataPacket_<2193>, bp::ClientboundUpdateSoundDataPacket_<2225>>::transform(
    Context<bp::ClientboundUpdateSoundDataPacket_<2225>> &ctx, bp::ClientboundUpdateSoundDataPacket_<2193> &&from)
{
    auto &to = ctx.out();
    to.server_sound_handle = from.server_sound_handle;
    // ENDWEAVE: 2193 writes the one event into all seven slots, and its client keeps the last.
    to.event = ew::transform(ctx, std::move(from.resume));
}

} // namespace endweave
