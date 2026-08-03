#pragma once

#include "endweave/protocol/protocol.h"

#include <array>
#include <bedrock/packet.hpp>
#include <type_traits>

namespace endweave {

/**
 * Drops packets the step cannot carry across, usually ones the older version never had.
 *
 * @note endweave-specific. ViaVersion spells this cancelClientbound(id) inside registerPackets().
 */
template <auto... Ids>
struct Cancel {
    static constexpr std::array<int, sizeof...(Ids)> ids{static_cast<int>(Ids)...};

    static constexpr bool covers(int packet_id)
    {
        return ((static_cast<int>(Ids) == packet_id) || ...);
    }
};

/** @note endweave-specific, alongside Cancel. */
template <auto... Ids>
inline constexpr Cancel<Ids...> cancel{};

/**
 * Forwards reshaped packets as they arrived, acknowledging in writing that no converter exists.
 * The receiver may misread them.
 *
 * @note endweave-specific. ViaVersion has nothing to acknowledge, since an unwritten remapper is
 * indistinguishable from a packet that did not change.
 */
template <auto... Ids>
struct Unconverted {
    static constexpr bool covers(int packet_id)
    {
        return ((static_cast<int>(Ids) == packet_id) || ...);
    }
};

/** @note endweave-specific, alongside Unconverted. */
template <auto... Ids>
inline constexpr Unconverted<Ids...> unconverted{};

/**
 * What a node does with each packet one step of its edge touches: converters, plus a cancel<...>
 * or unconverted<...> for the rest. A converter names the two shapes it maps between, so the id
 * it covers is In::Id and is never written twice.
 *
 * @note endweave-specific. ViaVersion registers imperatively in registerPackets(), and a function
 * body is not something the compiler can read back to check a reshaped packet was covered.
 * @see ViaVersion AbstractProtocol#clientboundMappings / #serverboundMappings, which this fills.
 */
template <auto... Entries>
struct Mappings {
    /** The same list plus More, for the step that differs from its opposite by a little. */
    template <auto... More>
    using And = Mappings<Entries..., More...>;
};

/**
 * Named in the error when a packet reshaped across a node's edge and the node declared nothing
 * for it. Declared and never defined, so the diagnostic carries the packet type.
 *
 * @note endweave-specific.
 */
template <class Packet>
struct MissingUpgradeConverterFor;

/** @note endweave-specific, alongside MissingUpgradeConverterFor. */
template <class Packet>
struct MissingDowngradeConverterFor;

namespace detail {

/** The two shapes a converter maps between, read off its signature. */
template <auto Entry>
struct ConverterTraits;

template <class In, class Out, std::expected<Out, PacketError> (*Converter)(UserConnection &, const In &)>
struct ConverterTraits<Converter> {
    using in = In;
    using out = Out;
};

template <class Entry>
inline constexpr bool is_cancel_v = false;
template <auto... Ids>
inline constexpr bool is_cancel_v<Cancel<Ids...>> = true;

template <class Entry>
inline constexpr bool is_unconverted_v = false;
template <auto... Ids>
inline constexpr bool is_unconverted_v<Unconverted<Ids...>> = true;

/** Whether one entry accounts for a packet id, whichever kind of entry it is. */
template <auto Entry>
constexpr bool entryCovers(int packet_id)
{
    using E = std::remove_cvref_t<decltype(Entry)>;
    if constexpr (is_cancel_v<E> || is_unconverted_v<E>) {
        return E::covers(packet_id);
    }
    else {
        return ConverterTraits<Entry>::in::Id == packet_id;
    }
}

template <class List>
struct Declared;

template <auto... Entries>
struct Declared<Mappings<Entries...>> {
    static constexpr bool covers(int packet_id)
    {
        return (entryCovers<Entries>(packet_id) || ...);
    }
};

template <class Node>
constexpr auto upgradesOf()
{
    if constexpr (requires { typename Node::Upgrades; }) {
        return typename Node::Upgrades{};
    }
    else {
        return Mappings<>{};
    }
}

template <class Node>
constexpr auto downgradesOf()
{
    if constexpr (requires { typename Node::Downgrades; }) {
        return typename Node::Downgrades{};
    }
    else {
        return Mappings<>{};
    }
}

template <class Node>
using UpgradesOf = decltype(upgradesOf<Node>());

template <class Node>
using DowngradesOf = decltype(downgradesOf<Node>());

/**
 * Whether a packet id names a different C++ type at the two versions. A packet only one of them
 * has counts, since appearing and disappearing are shape changes of the strongest kind.
 */
template <int Prev, int Cur>
constexpr bool reshaped(int packet_id)
{
    namespace bp = bedrock::protocol;
    const bool at_prev = bp::Registry<Prev>::contains(packet_id);
    const bool at_cur = bp::Registry<Cur>::contains(packet_id);
    if (at_prev != at_cur) {
        return true;
    }
    if (!at_prev) {
        return false;
    }
    bool same = false;
    bp::Registry<Prev>::visit(packet_id, [&]<class Before>() {
        bp::Registry<Cur>::visit(packet_id, [&]<class After>() {
            same = std::is_same_v<Before, After>;
        });
    });
    return !same;
}

/**
 * Requires a declaration in Covered for every packet in the list the edge reshapes, naming
 * Missing<Packet> where there is none. The caller passes the packets the step reads.
 */
template <template <class> class Missing, class Covered, int Prev, int Cur, class... Packets>
constexpr void check(bedrock::protocol::PacketList<Packets...>)
{
    (
        [] {
            if constexpr (reshaped<Prev, Cur>(Packets::Id) && !Declared<Covered>::covers(Packets::Id)) {
                static_cast<void>(sizeof(Missing<Packets>));
            }
        }(),
        ...);
}

} // namespace detail

/**
 * The base every version node derives from. It checks at compile time that the node covers every
 * packet its edge reshapes, then fills the handler tables from what the node declared, so the two
 * cannot drift.
 *
 * A node declares `using Upgrades = Mappings<...>` and `using Downgrades = Mappings<...>`, each
 * optional. Anything a step must amend on an id already covered goes in registerExtras().
 *
 * @note endweave-specific. ViaVersion has no such base, because an imperative registerPackets()
 * gives the compiler nothing to check.
 * @see ViaVersion AbstractProtocol#registerPackets, which this calls in place of.
 */
template <ProtocolVersion V, class Derived>
class VersionNode : public AbstractProtocol {
    static_assert(isModelled(V), "a version node must be listed in kProtocolVersions");

public:
    VersionNode() : AbstractProtocol(V) {}

protected:
    /** @see ViaVersion AbstractProtocol#registerPackets. */
    void registerPackets() final
    {
        constexpr int previous = static_cast<int>(previousOf(V));
        constexpr int current = static_cast<int>(V);

        // An upgrade reads the older shape, a downgrade the newer one.
        detail::check<MissingUpgradeConverterFor, detail::UpgradesOf<Derived>, previous, current>(
            typename bedrock::protocol::Registry<previous>::types{});
        detail::check<MissingDowngradeConverterFor, detail::DowngradesOf<Derived>, previous, current>(
            typename bedrock::protocol::Registry<current>::types{});

        registerMappings(Step::Upgrade, detail::UpgradesOf<Derived>{});
        registerMappings(Step::Downgrade, detail::DowngradesOf<Derived>{});

        if constexpr (requires(Derived &node) { node.registerExtras(); }) {
            static_cast<Derived *>(this)->registerExtras();
        }
    }

private:
    template <auto... Entries>
    void registerMappings(Step step, Mappings<Entries...>)
    {
        (registerEntry<Entries>(step), ...);
    }

    template <auto Entry>
    void registerEntry(Step step)
    {
        using E = std::remove_cvref_t<decltype(Entry)>;
        if constexpr (detail::is_unconverted_v<E>) {
            (void)step; // the body is forwarded as it arrived, so there is nothing to register
        }
        else if constexpr (detail::is_cancel_v<E>) {
            for (const int id : E::ids) {
                const auto packet_id = static_cast<bedrock::protocol::MinecraftPacketIds>(id);
                if (step == Step::Upgrade) {
                    this->cancelUpgrade(packet_id);
                }
                else {
                    this->cancelDowngrade(packet_id);
                }
            }
        }
        else if (step == Step::Upgrade) {
            this->registerUpgrade(Entry);
        }
        else {
            this->registerDowngrade(Entry);
        }
    }
};

} // namespace endweave
