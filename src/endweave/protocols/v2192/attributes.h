#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/attributes.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::EnvironmentAttributeData_<2192>, bp::EnvironmentAttributeData_<2168>> {
    static void transform(Context<bp::EnvironmentAttributeData_<2168>> &ctx,
                          bp::EnvironmentAttributeData_<2192> &&from);
};

template <>
struct Transformer<bp::AttributeLayerData_<2192>, bp::AttributeLayerData_<2168>> {
    static void transform(Context<bp::AttributeLayerData_<2168>> &ctx, bp::AttributeLayerData_<2192> &&from);
};

template <>
struct Transformer<bp::UpdateAttributeLayersData_<2192>, bp::UpdateAttributeLayersData_<2168>> {
    static void transform(Context<bp::UpdateAttributeLayersData_<2168>> &ctx,
                          bp::UpdateAttributeLayersData_<2192> &&from);
};

template <>
struct Transformer<bp::UpdateEnvironmentAttributesData_<2192>, bp::UpdateEnvironmentAttributesData_<2168>> {
    static void transform(Context<bp::UpdateEnvironmentAttributesData_<2168>> &ctx,
                          bp::UpdateEnvironmentAttributesData_<2192> &&from);
};

template <>
struct Transformer<bp::ClientboundAttributeLayerSyncPacket_<2192>, bp::ClientboundAttributeLayerSyncPacket_<2168>> {
    static void transform(Context<bp::ClientboundAttributeLayerSyncPacket_<2168>> &ctx,
                          bp::ClientboundAttributeLayerSyncPacket_<2192> &&from);
};

} // namespace endweave
