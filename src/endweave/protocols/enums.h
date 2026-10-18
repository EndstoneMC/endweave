#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/enum.hpp>
#include <string_view>
#include <type_traits>

namespace bp = bedrock::protocol;

namespace endweave {

namespace detail {

template <class From, class To>
concept ReflectedEnums = std::is_enum_v<From> && std::is_enum_v<To> && requires {
    bp::enum_type_name<From>();
    bp::enum_type_name<To>();
};

/** Whether `From` and `To` are two versions of the enum bedrock-protocol reflects as `name`. */
template <class From, class To>
consteval bool versionsOf(std::string_view name)
{
    if constexpr (ReflectedEnums<From, To>) {
        return !std::is_same_v<From, To> && bp::enum_type_name<From>() == name && bp::enum_type_name<To>() == name;
    }
    else {
        return false;
    }
}

} // namespace detail

// ENDWEAVE: an actor type one side has alone lands on Undefined.
template <class From, class To>
    requires(detail::versionsOf<From, To>("ActorType"))
struct Transformer<From, To> {
    static void transform(Context<To> &ctx, From &&from)
    {
        ctx.out() = bp::enum_cast<To>(bp::enum_name(from)).value_or(To::Undefined);
    }
};

// ENDWEAVE: a persona piece one side has alone lands on Unsupported.
template <class From, class To>
    requires(detail::versionsOf<From, To>("PieceType"))
struct Transformer<From, To> {
    static void transform(Context<To> &ctx, From &&from)
    {
        ctx.out() = bp::enum_cast<To>(bp::enum_name(from)).value_or(To::Unsupported);
    }
};

// ENDWEAVE: a slot in a container the other side does not name has nowhere to land that is not some
// other container's, so the packet is dropped.
template <class From, class To>
    requires(detail::versionsOf<From, To>("ContainerEnumName"))
struct Transformer<From, To> {
    static void transform(Context<To> &ctx, From &&from)
    {
        const auto name = bp::enum_cast<To>(bp::enum_name(from));
        if (!name) {
            ctx.cancel();
            return;
        }
        ctx.out() = *name;
    }
};

// ENDWEAVE: a container the other side does not name has no screen there, and opening the wrong one
// is further from what the server meant than opening nothing.
template <class From, class To>
    requires(detail::versionsOf<From, To>("ContainerType"))
struct Transformer<From, To> {
    static void transform(Context<To> &ctx, From &&from)
    {
        const auto type = bp::enum_cast<To>(bp::enum_name(from));
        if (!type) {
            ctx.cancel();
            return;
        }
        ctx.out() = *type;
    }
};

// ENDWEAVE: a command version the other side does not name is newer than anything it knows, so the
// command runs as the latest one it has.
template <class From, class To>
    requires(detail::versionsOf<From, To>("CurrentCmdVersion"))
struct Transformer<From, To> {
    static void transform(Context<To> &ctx, From &&from)
    {
        ctx.out() = bp::enum_cast<To>(bp::enum_name(from)).value_or(To::Latest);
    }
};

// ENDWEAVE: a reason one side has alone lands on Unknown; the message still says why in words.
template <class From, class To>
    requires(detail::versionsOf<From, To>("DisconnectFailReason"))
struct Transformer<From, To> {
    static void transform(Context<To> &ctx, From &&from)
    {
        ctx.out() = bp::enum_cast<To>(bp::enum_name(from)).value_or(To::Unknown);
    }
};

// ENDWEAVE: a packet one side has alone names no packet on the other and falls back to the sentinel
// past the last id.
template <class From, class To>
    requires(detail::versionsOf<From, To>("MinecraftPacketIds"))
struct Transformer<From, To> {
    static void transform(Context<To> &ctx, From &&from)
    {
        ctx.out() = bp::enum_cast<To>(bp::enum_name(from)).value_or(To::EndId);
    }
};

// ENDWEAVE: a memory category one side has alone lands on Unknown.
template <class From, class To>
    requires(detail::versionsOf<From, To>("MemoryCategory"))
struct Transformer<From, To> {
    static void transform(Context<To> &ctx, From &&from)
    {
        ctx.out() = bp::enum_cast<To>(bp::enum_name(from)).value_or(To::Unknown);
    }
};

// ENDWEAVE: an event for an achievement the other side has no name for is dropped rather than
// reported as another.
template <class From, class To>
    requires(detail::versionsOf<From, To>("MinecraftEventing::AchievementIds"))
struct Transformer<From, To> {
    static void transform(Context<To> &ctx, From &&from)
    {
        const auto achievement = bp::enum_cast<To>(bp::enum_name(from));
        if (!achievement) {
            ctx.cancel();
            return;
        }
        ctx.out() = *achievement;
    }
};

template <class From, class To>
    requires(detail::versionsOf<From, To>("MinecraftEventing::InteractionType"))
struct Transformer<From, To> {
    static void transform(Context<To> &ctx, From &&from)
    {
        const auto interaction = bp::enum_cast<To>(bp::enum_name(from));
        if (!interaction) {
            ctx.cancel();
            return;
        }
        ctx.out() = *interaction;
    }
};

// ENDWEAVE: an input flag the other side lacks lands on the InputNum sentinel rather than dropping the
// whole tick of input.
template <class From, class To>
    requires(detail::versionsOf<From, To>("PlayerAuthInputPacket::InputData"))
struct Transformer<From, To> {
    static void transform(Context<To> &ctx, From &&from)
    {
        ctx.out() = bp::enum_cast<To>(bp::enum_name(from)).value_or(To::InputNum);
    }
};

template <class From, class To>
    requires(detail::versionsOf<From, To>("ItemUseInventoryTransaction::ActionType"))
struct Transformer<From, To> {
    static void transform(Context<To> &ctx, From &&from)
    {
        const auto action = bp::enum_cast<To>(bp::enum_name(from));
        if (!action) {
            ctx.cancel();
            return;
        }
        ctx.out() = *action;
    }
};

template <class From, class To>
    requires(detail::versionsOf<From, To>("SubChunkPacket::HeightMapDataType"))
struct Transformer<From, To> {
    static void transform(Context<To> &ctx, From &&from)
    {
        ctx.out() = bp::enum_cast<To>(bp::enum_name(from)).value_or(To::NoData);
    }
};

} // namespace endweave
