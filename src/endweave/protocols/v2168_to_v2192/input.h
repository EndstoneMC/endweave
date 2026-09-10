#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/input.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::PlayerAuthInputPacket_<2168>::InputData, bp::PlayerAuthInputPacket_<2192>::InputData> {
    static void transform(Context<bp::PlayerAuthInputPacket_<2192>::InputData> &ctx,
                          bp::PlayerAuthInputPacket_<2168>::InputData &&from);
};
} // namespace endweave
