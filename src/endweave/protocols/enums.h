#pragma once

#include "endweave/rewriter/enum.h"

#include <bedrock/protocol/actor.h>
#include <bedrock/protocol/command.h>
#include <bedrock/protocol/diagnostics.h>
#include <bedrock/protocol/inventory.h>
#include <bedrock/protocol/map.h>
#include <bedrock/protocol/network.h>
#include <bedrock/protocol/skin.h>
#include <bedrock/protocol/telemetry.h>
#include <optional>

namespace bp = bedrock::protocol;

namespace endweave {

// ENDWEAVE: an actor type the destination lacks lands on Undefined.
template <>
struct EnumPolicy<bp::ActorType> {
    static constexpr std::optional<bp::ActorType> apply(bp::ActorType)
    {
        return bp::ActorType::Undefined;
    }
};

// ENDWEAVE: a persona piece the destination lacks lands on Unsupported.
template <>
struct EnumPolicy<bp::PieceType> {
    static constexpr std::optional<bp::PieceType> apply(bp::PieceType)
    {
        return bp::PieceType::Unsupported;
    }
};

// ENDWEAVE: a slot in a container the destination does not name has nowhere to land that is not some
// other container's, so the packet is dropped.
template <>
struct EnumPolicy<bp::ContainerEnumName> {
    static constexpr std::optional<bp::ContainerEnumName> apply(bp::ContainerEnumName)
    {
        return std::nullopt;
    }
};

// ENDWEAVE: a container the destination does not name has no screen there, and opening the wrong one
// is further from what the server meant than opening nothing.
template <>
struct EnumPolicy<bp::ContainerType> {
    static constexpr std::optional<bp::ContainerType> apply(bp::ContainerType)
    {
        return std::nullopt;
    }
};

// ENDWEAVE: a command version the destination does not name runs as the latest one it has.
template <>
struct EnumPolicy<bp::CurrentCmdVersion> {
    static constexpr std::optional<bp::CurrentCmdVersion> apply(bp::CurrentCmdVersion)
    {
        return bp::CurrentCmdVersion::Latest;
    }
};

// ENDWEAVE: a reason the destination lacks lands on Unknown; the message still says why in words.
template <>
struct EnumPolicy<bp::DisconnectFailReason> {
    static constexpr std::optional<bp::DisconnectFailReason> apply(bp::DisconnectFailReason)
    {
        return bp::DisconnectFailReason::Unknown;
    }
};

// ENDWEAVE: a structure marker the destination lacks borrows the closest-looking one it has, and any
// other image it lacks is not drawn.
template <>
struct EnumPolicy<bp::MapDecoration::Type> {
    static constexpr std::optional<bp::MapDecoration::Type> apply(bp::MapDecoration::Type image)
    {
        using Type = bp::MapDecoration::Type;
        switch (image) {
        case Type::AbandonedCamp:
            return Type::WitchHut;
        case Type::BuriedAncientCity:
        case Type::BuriedMineshaft:
            return Type::TrialChambers;
        case Type::DesertPyramid:
            return Type::JungleTemple;
        case Type::WarmOceanRuins:
            return Type::Monument;
        default:
            return Type::NoDraw;
        }
    }
};

// ENDWEAVE: a memory category the destination lacks lands on Unknown.
template <>
struct EnumPolicy<bp::MemoryCategory> {
    static constexpr std::optional<bp::MemoryCategory> apply(bp::MemoryCategory)
    {
        return bp::MemoryCategory::Unknown;
    }
};

// ENDWEAVE: an event for an achievement the destination has no number for is dropped rather than
// reported as another.
template <>
struct EnumPolicy<bp::MinecraftEventing::AchievementIds> {
    static constexpr std::optional<bp::MinecraftEventing::AchievementIds> apply(bp::MinecraftEventing::AchievementIds)
    {
        return std::nullopt;
    }
};

// ENDWEAVE: a packet the destination lacks names no packet there, so a report about one is dropped.
template <>
struct EnumPolicy<bp::MinecraftPacketIds> {
    static constexpr std::optional<bp::MinecraftPacketIds> apply(bp::MinecraftPacketIds)
    {
        return std::nullopt;
    }
};

} // namespace endweave
