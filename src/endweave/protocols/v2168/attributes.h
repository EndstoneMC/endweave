#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/attributes.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::EnvironmentAttributeData_<2168>, bp::EnvironmentAttributeData_<2181>> {
    static bp::EnvironmentAttributeData_<2181> transform(bp::EnvironmentAttributeData_<2168> &&from);
};

template <>
struct Transformer<bp::AttributeLayerData_<2168>, bp::AttributeLayerData_<2181>> {
    static bp::AttributeLayerData_<2181> transform(bp::AttributeLayerData_<2168> &&from);
};

template <>
struct Transformer<bp::UpdateAttributeLayersData_<2168>, bp::UpdateAttributeLayersData_<2181>> {
    static bp::UpdateAttributeLayersData_<2181> transform(bp::UpdateAttributeLayersData_<2168> &&from);
};

template <>
struct Transformer<bp::UpdateEnvironmentAttributesData_<2168>, bp::UpdateEnvironmentAttributesData_<2181>> {
    static bp::UpdateEnvironmentAttributesData_<2181> transform(bp::UpdateEnvironmentAttributesData_<2168> &&from);
};

template <>
struct Transformer<bp::ClientboundAttributeLayerSyncPacket_<2168>, bp::ClientboundAttributeLayerSyncPacket_<2181>> {
    static bp::ClientboundAttributeLayerSyncPacket_<2181> transform(
        bp::ClientboundAttributeLayerSyncPacket_<2168> &&from);
};

} // namespace endweave
