#include "endweave/protocols/v2168/block.h"

namespace endweave {

void Transformer<bp::AnvilDamagePacket_<2168>, bp::AnvilDamagePacket_<1001>>::transform(
    Context<bp::AnvilDamagePacket_<1001>> &ctx, bp::AnvilDamagePacket_<2168> &&from)
{
    auto &to = ctx.out();
    // ENDWEAVE: 2168 sends no damage and it cannot be recovered. Undamaged is the one value that
    // cannot break an anvil the server still has.
    to.damage = 0;
    to.position = from.position;
}

} // namespace endweave
