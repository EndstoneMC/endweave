#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/item_stack.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::ItemStackRequestCereal_<2193>::RequestData, bp::ItemStackRequestCereal_<2225>::RequestData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2225>::RequestData> &ctx,
                          bp::ItemStackRequestCereal_<2193>::RequestData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestPacketData_<2193>::RequestData,
                   bp::ItemStackRequestPacketData_<2225>::RequestData> {
    static void transform(Context<bp::ItemStackRequestPacketData_<2225>::RequestData> &ctx,
                          bp::ItemStackRequestPacketData_<2193>::RequestData &&from);
};
} // namespace endweave
