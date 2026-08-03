#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/inventory.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::v2168::SerializedNetworkItemStackDescriptor> {
    static bp::v1001::SerializedNetworkItemStackDescriptor transform(
        bp::v2168::SerializedNetworkItemStackDescriptor &&from);
};

} // namespace endweave
