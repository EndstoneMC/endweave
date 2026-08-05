#include "endweave/protocols/v2168/block.h"

namespace endweave {

bp::AnvilDamagePacket_<1001> Transformer<bp::AnvilDamagePacket_<2168>, bp::AnvilDamagePacket_<1001>>::transform(
    bp::AnvilDamagePacket_<2168> &&from)
{
    bp::AnvilDamagePacket_<1001> to;
    // ENDWEAVE: TODO 2168 sends no damage and it cannot be recovered. Undamaged is the one value that
    // cannot break an anvil the server still has.
    to.damage = 0;
    to.position = from.position;
    return to;
}

} // namespace endweave
