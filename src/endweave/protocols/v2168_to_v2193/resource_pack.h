#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/pack.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::ServerboundPackSettingChangePacket_<2168>, bp::ServerboundPackSettingChangePacket_<2193>> {
    static void transform(Context<bp::ServerboundPackSettingChangePacket_<2193>> &ctx,
                          bp::ServerboundPackSettingChangePacket_<2168> &&from);
};

} // namespace endweave
