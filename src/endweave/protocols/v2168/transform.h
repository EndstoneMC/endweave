#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/game.h>
#include <protocol/inventory.h>
#include <protocol/presence.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::v2168::SerializedNetworkItemStackDescriptor> {
    static bp::v1001::SerializedNetworkItemStackDescriptor downgrade(
        bp::v2168::SerializedNetworkItemStackDescriptor &&from);
};

template <>
struct Transformer<bp::v2168::InventoryContentPacket> {
    static bp::v1001::InventoryContentPacket downgrade(bp::v2168::InventoryContentPacket &&from);
};

template <>
struct Transformer<bp::ExperimentToggle> {
    static bp::ExperimentData downgrade(bp::ExperimentToggle &&from);
};

template <>
struct Transformer<bp::v2168::LevelSettings> {
    static bp::v1001::LevelSettings downgrade(bp::v2168::LevelSettings &&from);
};

template <>
struct Transformer<bp::v2168::ServerBlockProperty> {
    static bp::BlockEntry downgrade(bp::v2168::ServerBlockProperty &&from);
};

template <>
struct Transformer<bp::v2168::PresenceConfiguration> {
    static bp::v1001::PresenceConfiguration downgrade(bp::v2168::PresenceConfiguration &&from);
};

template <>
struct Transformer<bp::v2168::GatheringsConfigurationJoinInfo> {
    static bp::v1001::GatheringsConfigurationJoinInfo downgrade(bp::v2168::GatheringsConfigurationJoinInfo &&from);
};

template <>
struct Transformer<bp::v2168::ServerConfigurationJoinInfo> {
    static bp::v1001::ServerConfigurationJoinInfo downgrade(bp::v2168::ServerConfigurationJoinInfo &&from);
};

template <>
struct Transformer<bp::v2168::StartGamePacket> {
    static bp::v1001::StartGamePacket downgrade(bp::v2168::StartGamePacket &&from);
};

} // namespace endweave
