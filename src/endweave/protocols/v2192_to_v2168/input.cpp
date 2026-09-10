#include "input.h"

#include <bedrock/protocol/enum.hpp>

namespace endweave {

void Transformer<bp::PlayerAuthInputPacket_<2192>::InputData, bp::PlayerAuthInputPacket_<2168>::InputData>::transform(
    Context<bp::PlayerAuthInputPacket_<2168>::InputData> &ctx, bp::PlayerAuthInputPacket_<2192>::InputData &&from)
{
    using To = bp::PlayerAuthInputPacket_<2168>::InputData;
    ctx.out() = bp::enum_cast<To>(bp::enum_name(from)).value_or(To::InputNum);
}

} // namespace endweave
