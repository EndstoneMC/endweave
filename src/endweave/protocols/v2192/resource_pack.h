#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/resource_pack.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::ServerboundPackSettingChangePacket_<2192>, bp::ServerboundPackSettingChangePacket_<2168>> {
    static void transform(Context<bp::ServerboundPackSettingChangePacket_<2168>> &ctx,
                          bp::ServerboundPackSettingChangePacket_<2192> &&from);
};

} // namespace endweave
