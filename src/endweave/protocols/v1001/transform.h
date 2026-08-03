#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/inventory.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::v1001::SerializedNetworkItemStackDescriptor> {
    static bp::v2168::SerializedNetworkItemStackDescriptor transform(
        bp::v1001::SerializedNetworkItemStackDescriptor &&from);
};

} // namespace endweave
