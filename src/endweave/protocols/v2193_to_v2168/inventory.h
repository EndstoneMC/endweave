#pragma once

#include "endweave/protocol/transform.h"
#include "endweave/protocols/enums.h"

#include <bedrock/protocol/transaction.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::ItemUseInventoryTransaction_<2193>, bp::ItemUseInventoryTransaction_<2168>> {
    static void transform(Context<bp::ItemUseInventoryTransaction_<2168>> &ctx,
                          bp::ItemUseInventoryTransaction_<2193> &&from);
};

} // namespace endweave
