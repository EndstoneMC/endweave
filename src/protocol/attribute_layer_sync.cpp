#include "endweave/protocol/attribute_layer_sync.h"

#include <variant>

namespace endweave {
namespace {

namespace bp = bedrock::protocol;

// The 976 wire diff is confined to two nested types. Everything else in the
// closure (AttributeDataVariant, AttributeLayerSettings, DimensionType, the
// UpdateAttributeLayerSettings/RemoveEnvironmentAttributes union cases) is
// version-shared and copies straight across.

// EnvironmentAttributeData: gains local_transition_ticks + noise_transition.
bp::v1001::EnvironmentAttributeData up(const bp::base::EnvironmentAttributeData &in)
{
    bp::v1001::EnvironmentAttributeData out;
    out.name = in.name;
    out.from_attribute = in.from_attribute;
    out.attribute = in.attribute;
    out.to_attribute = in.to_attribute;
    out.current_transition_ticks = in.current_transition_ticks;
    out.total_transition_ticks = in.total_transition_ticks;
    out.easing = in.easing;
    out.local_transition_ticks = 0;  // polyfill
    out.noise_transition = false;    // polyfill
    return out;
}

bp::base::EnvironmentAttributeData down(const bp::v1001::EnvironmentAttributeData &in)
{
    bp::base::EnvironmentAttributeData out;
    out.name = in.name;
    out.from_attribute = in.from_attribute;
    out.attribute = in.attribute;
    out.to_attribute = in.to_attribute;
    out.current_transition_ticks = in.current_transition_ticks;
    out.total_transition_ticks = in.total_transition_ticks;
    out.easing = in.easing;
    // local_transition_ticks, noise_transition dropped (lossy)
    return out;
}

// AttributeLayerData: gains noise_name.
bp::v1001::AttributeLayerData up(const bp::base::AttributeLayerData &in)
{
    bp::v1001::AttributeLayerData out;
    out.name = in.name;
    out.noise_name = std::nullopt;  // polyfill
    out.dimension_id = in.dimension_id;
    out.settings = in.settings;
    out.attributes.reserve(in.attributes.size());
    for (const auto &e : in.attributes) {
        out.attributes.push_back(up(e));
    }
    return out;
}

bp::base::AttributeLayerData down(const bp::v1001::AttributeLayerData &in)
{
    bp::base::AttributeLayerData out;
    out.name = in.name;
    // noise_name dropped (lossy)
    out.dimension_id = in.dimension_id;
    out.settings = in.settings;
    out.attributes.reserve(in.attributes.size());
    for (const auto &e : in.attributes) {
        out.attributes.push_back(down(e));
    }
    return out;
}

// Union case 0: UpdateAttributeLayersData.
bp::v1001::UpdateAttributeLayersData up(const bp::base::UpdateAttributeLayersData &in)
{
    bp::v1001::UpdateAttributeLayersData out;
    out.attribute_layers.reserve(in.attribute_layers.size());
    for (const auto &l : in.attribute_layers) {
        out.attribute_layers.push_back(up(l));
    }
    return out;
}

bp::base::UpdateAttributeLayersData down(const bp::v1001::UpdateAttributeLayersData &in)
{
    bp::base::UpdateAttributeLayersData out;
    out.attribute_layers.reserve(in.attribute_layers.size());
    for (const auto &l : in.attribute_layers) {
        out.attribute_layers.push_back(down(l));
    }
    return out;
}

// Union case 2: UpdateEnvironmentAttributesData.
bp::v1001::UpdateEnvironmentAttributesData up(const bp::base::UpdateEnvironmentAttributesData &in)
{
    bp::v1001::UpdateEnvironmentAttributesData out;
    out.layer_name = in.layer_name;
    out.layer_dimension_id = in.layer_dimension_id;
    out.attributes.reserve(in.attributes.size());
    for (const auto &e : in.attributes) {
        out.attributes.push_back(up(e));
    }
    return out;
}

bp::base::UpdateEnvironmentAttributesData down(const bp::v1001::UpdateEnvironmentAttributesData &in)
{
    bp::base::UpdateEnvironmentAttributesData out;
    out.layer_name = in.layer_name;
    out.layer_dimension_id = in.layer_dimension_id;
    out.attributes.reserve(in.attributes.size());
    for (const auto &e : in.attributes) {
        out.attributes.push_back(down(e));
    }
    return out;
}

}  // namespace

AttributeLayerSyncV1001 upgrade(const AttributeLayerSyncV975 &packet)
{
    AttributeLayerSyncV1001 out;
    switch (packet.data.index()) {
    case 0:
        out.data = up(std::get<0>(packet.data));
        break;
    case 1:
        out.data = std::get<1>(packet.data);  // version-shared
        break;
    case 2:
        out.data = up(std::get<2>(packet.data));
        break;
    case 3:
        out.data = std::get<3>(packet.data);  // version-shared
        break;
    }
    return out;
}

AttributeLayerSyncV975 downgrade(const AttributeLayerSyncV1001 &packet)
{
    AttributeLayerSyncV975 out;
    switch (packet.data.index()) {
    case 0:
        out.data = down(std::get<0>(packet.data));
        break;
    case 1:
        out.data = std::get<1>(packet.data);  // version-shared
        break;
    case 2:
        out.data = down(std::get<2>(packet.data));
        break;
    case 3:
        out.data = std::get<3>(packet.data);  // version-shared
        break;
    }
    return out;
}

}  // namespace endweave
