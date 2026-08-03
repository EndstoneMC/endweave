#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/game.h>
#include <protocol/inventory.h>
#include <protocol/presence.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::v2168::SerializedNetworkItemStackDescriptor> {
    static bp::v1001::SerializedNetworkItemStackDescriptor transform(
        bp::v2168::SerializedNetworkItemStackDescriptor &&from);
};

template <>
struct Transformer<bp::v2168::InventoryContentPacket> {
    static bp::v1001::InventoryContentPacket transform(bp::v2168::InventoryContentPacket &&from);
};

template <>
struct Transformer<bp::ExperimentToggle> {
    static bp::ExperimentData transform(bp::ExperimentToggle &&from);
};

template <>
struct Transformer<bp::v2168::LevelSettings> {
    static bp::v1001::LevelSettings transform(bp::v2168::LevelSettings &&from);
};

template <>
struct Transformer<bp::v2168::ServerBlockProperty> {
    static bp::BlockEntry transform(bp::v2168::ServerBlockProperty &&from);
};

template <>
struct Transformer<bp::v2168::PresenceConfiguration> {
    static bp::v1001::PresenceConfiguration transform(bp::v2168::PresenceConfiguration &&from);
};

template <>
struct Transformer<bp::v2168::GatheringsConfigurationJoinInfo> {
    static bp::v1001::GatheringsConfigurationJoinInfo transform(bp::v2168::GatheringsConfigurationJoinInfo &&from);
};

template <>
struct Transformer<bp::v2168::ServerConfigurationJoinInfo> {
    static bp::v1001::ServerConfigurationJoinInfo transform(bp::v2168::ServerConfigurationJoinInfo &&from);
};

template <>
struct Transformer<bp::v2168::StartGamePacket> {
    static bp::v1001::StartGamePacket transform(bp::v2168::StartGamePacket &&from);
};

} // namespace endweave
