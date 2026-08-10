#include "endweave/protocols/v1001/block.h"

namespace endweave {

void Transformer<bp::AnvilDamagePacket_<1001>, bp::AnvilDamagePacket_<2168>>::transform(
    Context<bp::AnvilDamagePacket_<2168>> &ctx, bp::AnvilDamagePacket_<1001> &&from)
{
    auto &to = ctx.out();
    // ENDWEAVE: 2168 dropped the damage byte, so the client's claim is discarded rather than carried.
    to.position = from.position;
}

} // namespace endweave
