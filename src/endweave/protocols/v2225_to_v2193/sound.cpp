#include "sound.h"

#include <utility>

namespace ew = endweave;

namespace endweave {
void Transformer<bp::ClientboundUpdateSoundDataPacket_<2225>, bp::ClientboundUpdateSoundDataPacket_<2193>>::transform(
    Context<bp::ClientboundUpdateSoundDataPacket_<2193>> &ctx, bp::ClientboundUpdateSoundDataPacket_<2225> &&from)
{
    auto &to = ctx.out();
    to.server_sound_handle = from.server_sound_handle;
    // ENDWEAVE: 2193 carries the event once per slot, and BDS writes the same one into all seven.
    to.stop = ew::transform(ctx, std::move(from.event));
    to.set_volume = to.stop;
    to.set_pitch = to.stop;
    to.fade = to.stop;
    to.seek_to = to.stop;
    to.pause = to.stop;
    to.resume = to.stop;
}

} // namespace endweave
