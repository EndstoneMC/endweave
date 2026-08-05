#include "endweave/protocols/v1001/block.h"

namespace endweave {

bp::AnvilDamagePacket_<2168> Transformer<bp::AnvilDamagePacket_<1001>, bp::AnvilDamagePacket_<2168>>::transform(
    bp::AnvilDamagePacket_<1001> &&from)
{
    bp::AnvilDamagePacket_<2168> to;
    // ENDWEAVE: 2168 dropped the damage byte, so the client's claim is discarded rather than carried.
    to.position = from.position;
    return to;
}

} // namespace endweave
