# endweave

A protocol-translation plugin for [Endstone](https://github.com/EndstoneMC/endstone).
endweave lets clients on an older Minecraft: Bedrock protocol join a server running a newer
one (and vice versa) by translating packets on the wire.

The wire codec lives in the sibling [bedrock-protocol](https://github.com/EndstoneMC/bedrock-protocol)
library, which generates a C++ packet type per protocol version from a Python schema. endweave
owns the *translation* semantics: what to do when a field is added, removed, or reshaped
between versions.

**No packets are translated across versions yet.** The framework below — the version graph, the
pipeline, the base protocol — is in place, and the base protocol decodes the handshake, but no
per-version converters are written, so every packet crosses a version step untouched. What has
changed is that the gaps are no longer invisible: a node must now account for every packet its
edge reshapes, and the build fails naming any it does not (see *Coverage is checked by the
compiler*).

## Design

A port of [ViaVersion](https://github.com/ViaVersion/ViaVersion), with one deliberate
difference.

ViaVersion has a protocol per adjacent-version *pair* (`Protocol1_21To1_20_5`), which makes a
protocol an **edge** in the version graph. endweave has one per *version*, so a protocol is a
**node**: `Protocol<V>` owns both directions of the wire diff between itself and the version
registered before it — the upgrade into V and the downgrade out of it. A version step is
therefore described once, in one directory, by the newer of its two ends, and nothing in the
tree is named after a pair of versions.

Because an endweave server may be older *or* newer than a given client, the same edge is
traversed in both transport directions across different connections. A node's two handler
tables are keyed by `Step { Upgrade, Downgrade }` rather than by clientbound/serverbound —
`Protocol<V>` is ViaVersion's forward protocol and ViaBackwards' backward protocol for one
step, fused into a single class:

| ViaVersion / ViaBackwards | endweave |
| --- | --- |
| `Protocol975To1001.registerServerbound` | `Protocol<V1001>::registerUpgrade` |
| `Protocol1001To975.registerClientbound` | `Protocol<V1001>::registerDowngrade` |

Everything else keeps ViaVersion's names and semantics: `AbstractProtocol`, `ProtocolManager`,
`ProtocolPipeline`, `UserConnection`, `ProtocolInfo`, `ConnectionManager`, `ProtocolPathEntry`,
`InitialBaseProtocol`, `PacketHandler`, `PacketHandlers`, `registerProtocols()`, and the
`initialize()` → `registerPackets()` → `init(UserConnection&)` lifecycle.

**ViaVersion's `PacketWrapper` is replaced by the codec.** It exists because ViaVersion reads
an untyped buffer through a runtime `Type<T>` registry; with a generated struct per era a
converter is a plain function of struct to struct, so the value list, the `Type` system and the
`Rewriter` hierarchy that held them all disappear. Cancellation comes back as a handler return
value — which is what ViaVersion's `cancelClientbound` amounts to anyway, since it registers
`PacketWrapper::cancel` as the handler.

A handler therefore names the two shapes it maps between, and the packet id follows from them:

```cpp
std::expected<bp::FooPacket_<1001>, PacketError> upgradeFoo(UserConnection &connection,
                                                            const bp::FooPacket_<975> &packet);
...
registerUpgrade(&upgradeFoo);   // id taken from FooPacket::Id
```

ViaVersion reports out of a handler by throwing, so where it throws endweave returns
`std::unexpected`: its `CancelException` is `PacketError::Cancelled` and its
`InformativeException` is the rest of the enum. `PacketError` is endweave's own type rather than
a `std::error_code`.

`ProtocolPipeline` decodes once and encodes once. A `PacketHolder` carries the body undecoded
until the first stage that handles the id asks for it as a type; later stages receive the struct
the previous one left behind, and the body is written back out only if some stage decoded it, so
a packet of no interest is forwarded byte for byte. A body the schema does not fully read is
caught rather than silently re-encoded truncated.

### Coverage is checked by the compiler

In ViaVersion "no remapper" genuinely means "no difference", because a remapper is the only place
a difference can be written down. Here the codec knows the shapes independently, so a reshaped
packet nobody converted would be forwarded silently, indistinguishable from one that never
changed. A node therefore *declares* what it covers, and the compiler checks the declaration:

```cpp
template <>
class Protocol<ProtocolVersion::v26_30> final
    : public VersionNode<ProtocolVersion::v26_30, Protocol<ProtocolVersion::v26_30>> {
    using Ids = bedrock::protocol::MinecraftPacketIds;

    using Reshaped = Mappings<&upgradeFoo, unconverted<Ids::START_GAME, Ids::BOSS_EVENT>>;

public:
    using Upgrades   = Reshaped;
    using Downgrades = Reshaped::And<cancel<Ids::CLIENTBOUND_UPDATE_SOUND_DATA>>;
};
```

An entry is a converter (whose id follows from `In::Id`), `cancel<Ids...>` for packets the other
version cannot carry, or `unconverted<Ids...>` to forward them as they arrived and admit it. A
reshape needs covering both ways, so both steps start from one list and `And<...>` adds whatever
is asymmetric. `VersionNode` pairs `Registry<prev>::types` against `Registry<cur>::types` by id and
demands a declaration wherever `std::is_same_v` fails or a packet exists on only one side, then
fills the handler tables from the same lists so the two cannot drift. A gap looks like this:

```
error: implicit instantiation of undefined template
  'endweave::MissingDowngradeConverterFor<bedrock::protocol::v1001::LevelSoundEventPacket>'
```

`unconverted<Ids...>` is not a loophole. It is the greppable admission the compiler forces someone
to write, and turning one into a converter is exactly what the translation work is.

**Dropped as Java-Edition-specific:** the `State` machine (Bedrock has one flat id space), the
per-version `PacketType` enums and name-based auto-mapping (Bedrock ids are stable and never
renumbered, so the id *is* the identity), the four generic parameters, and `MappingData`.

### The version graph

Nodes are protocols; edges join a node to the node registered before it. Registration order
*is* the graph, so protocols are registered in ascending version order:

```cpp
void ProtocolManager::registerProtocols()
{
    registerBaseProtocol(std::make_unique<InitialBaseProtocol>());
    registerProtocol<ProtocolVersion::V975>();
    registerProtocol<ProtocolVersion::V1001>();
}
```

`registerProtocol<V>()` constructs and owns the node, initialises it, inserts the vertex, and
derives both directions of the edge to its predecessor. Travelling *up* an edge uses the newer
node's `Upgrade` table, *down* its `Downgrade` table, so the edge itself carries which table to
use and the path search stays a plain breadth-first walk — a port of ViaVersion's
`calculateProtocolPath`, cache and depth fail-safe included.

Paths come back in serverbound order. A connection's pipeline runs that order forwards for
serverbound packets and backwards, with every step inverted, for clientbound ones; the base
protocol sits at the head of both and is never reversed.

### Adding a version

1. Add the enumerator to `ProtocolVersion` and append it to `kProtocolVersions`, in ascending order.
2. Add `src/endweave/protocols/vN/protocol.h` declaring `Protocol<ProtocolVersion::VN>` over
   `VersionNode`, with empty `Upgrades` and `Downgrades`.
3. Append one `registerProtocol<ProtocolVersion::VN>()` line to `registerProtocols()`.
4. Build. Every packet the new edge reshapes is now a named compile error. Work through them,
   adding a converter, a `cancel<Id>`, or an `unconverted<Id>` for each. Converters go in a
   sibling `protocol.cpp`.

## Layout

| path | |
| --- | --- |
| `src/endweave/protocol/` | the machinery: `AbstractProtocol`, the graph, the pipeline, the packet carrier, the handler DSL |
| `src/endweave/connection/` | `UserConnection`, `ProtocolInfo`, `ConnectionManager` |
| `src/endweave/protocols/` | one directory per node, plus `base/` for `InitialBaseProtocol` |
| `src/endweave/plugin.{h,cpp}` | the Endstone plugin, owning the registry and the connection table |
| `src/endweave/listener.{h,cpp}` | the packet events |
| `tests/` | Catch2 tests, plus a fixture under `negative/` that must *not* compile |

## Building

Requires CMake, Ninja, and Clang 18+ with libc++ (clang-cl on Windows), Endstone's own
toolchain floor, enforced by its CMake. bedrock-protocol is consumed from a sibling checkout by
default (`../bedrock-protocol`), and the Endstone SDK is fetched.

```shell
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake --build build
```

The plugin lands at `build/endstone_endweave.so`. Drop it in the server's `plugins/`.

```shell
ctest --test-dir build --output-on-failure
```
