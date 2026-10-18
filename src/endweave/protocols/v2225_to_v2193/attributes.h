#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/eas.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::EnvironmentAttributeData_<2225>, bp::EnvironmentAttributeData_<2193>> {
    static void transform(Context<bp::EnvironmentAttributeData_<2193>> &ctx,
                          bp::EnvironmentAttributeData_<2225> &&from);
};

template <>
struct Transformer<bp::AttributeLayerData_<2225>, bp::AttributeLayerData_<2193>> {
    static void transform(Context<bp::AttributeLayerData_<2193>> &ctx, bp::AttributeLayerData_<2225> &&from);
};
} // namespace endweave
