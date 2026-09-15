#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/eas.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::EnvironmentAttributeData_<2168>, bp::EnvironmentAttributeData_<2193>> {
    static void transform(Context<bp::EnvironmentAttributeData_<2193>> &ctx,
                          bp::EnvironmentAttributeData_<2168> &&from);
};
} // namespace endweave
