#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/game.h>
#include <protocol/inventory.h>
#include <protocol/presence.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::v1001::SerializedNetworkItemStackDescriptor> {
    static bp::v2168::SerializedNetworkItemStackDescriptor upgrade(
        bp::v1001::SerializedNetworkItemStackDescriptor &&from);
};

template <>
struct Transformer<bp::v1001::InventoryContentPacket> {
    static bp::v2168::InventoryContentPacket upgrade(bp::v1001::InventoryContentPacket &&from);
};

template <>
struct Transformer<bp::ExperimentData> {
    static bp::ExperimentToggle upgrade(bp::ExperimentData &&from);
};

template <>
struct Transformer<bp::v1001::LevelSettings> {
    static bp::v2168::LevelSettings upgrade(bp::v1001::LevelSettings &&from);
};

template <>
struct Transformer<bp::BlockEntry> {
    static bp::v2168::ServerBlockProperty upgrade(bp::BlockEntry &&from);
};

template <>
struct Transformer<bp::v1001::PresenceConfiguration> {
    static bp::v2168::PresenceConfiguration upgrade(bp::v1001::PresenceConfiguration &&from);
};

template <>
struct Transformer<bp::v1001::GatheringsConfigurationJoinInfo> {
    static bp::v2168::GatheringsConfigurationJoinInfo upgrade(bp::v1001::GatheringsConfigurationJoinInfo &&from);
};

template <>
struct Transformer<bp::v1001::ServerConfigurationJoinInfo> {
    static bp::v2168::ServerConfigurationJoinInfo upgrade(bp::v1001::ServerConfigurationJoinInfo &&from);
};

template <>
struct Transformer<bp::v1001::StartGamePacket> {
    static bp::v2168::StartGamePacket upgrade(bp::v1001::StartGamePacket &&from);
};

} // namespace endweave
