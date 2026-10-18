#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/eas.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::EnvironmentAttributeData_<2193>, bp::EnvironmentAttributeData_<2225>> {
    static void transform(Context<bp::EnvironmentAttributeData_<2225>> &ctx,
                          bp::EnvironmentAttributeData_<2193> &&from);
};

template <>
struct Transformer<bp::AttributeLayerData_<2193>, bp::AttributeLayerData_<2225>> {
    static void transform(Context<bp::AttributeLayerData_<2225>> &ctx, bp::AttributeLayerData_<2193> &&from);
};
} // namespace endweave
