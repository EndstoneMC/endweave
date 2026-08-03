# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

endweave is a protocol-translation plugin for [Endstone](https://github.com/EndstoneMC/endstone).
It lets a Minecraft: Bedrock client on an older (or newer) protocol join a server running a
different one by translating packets on the wire. The wire codec comes from the sibling
[bedrock-protocol](https://github.com/EndstoneMC/bedrock-protocol) library. endweave owns the
translation semantics.

## The prime directive: endweave is an exact port of ViaVersion

endweave is a faithful C++ port of [ViaVersion](https://github.com/ViaVersion/ViaVersion) (its
forward protocols) fused with [ViaBackwards](https://github.com/ViaVersion/ViaBackwards) (its
backward protocols). Maintainability against upstream is the point. A dev looking at any endweave
file should be able to find the ViaVersion class it came from, so that when upstream changes, the
corresponding endweave code can be updated in lockstep.

Treat the ViaVersion Java source as the specification. When in doubt, match it.

### Rules

1. **Mirror upstream structure, names, hierarchy, and members.** Classes keep their ViaVersion
   names, the inheritance tree matches (`AbstractProtocol` is the base, version protocols and the
   base protocol extend it), methods keep their upstream names and semantics, and member fields
   correspond one to one. Snake_case with a trailing underscore is the only concession, so
   `serverProtocolVersion` becomes `server_protocol_version_`.

2. **Do not add methods, helpers, or members that are not in upstream ViaVersion.** If you reach
   for something ViaVersion does not have, it must be one of:
   - **Platform glue**, the Endstone integration ViaVersion keeps in its own platform modules
     (the plugin main, the netty handlers, connection lifecycle). Lives in `plugin.*`,
     `listener.*`, and the address-keyed connection bookkeeping.
   - **A language-gap accommodation**, something the JVM gives ViaVersion for free that C++ must
     spell out: ownership (`std::unique_ptr` where Java relies on GC), the class name
     (`AbstractProtocol::name_` where Java uses `getClass().getSimpleName()`), a hash
     (`ProtocolPathKeyHash` where a Java record derives one).

   Anything in these two buckets **must be marked** with `@note endweave-specific.` naming what
   ViaVersion does instead. Everything else must trace back to an upstream member.

3. **No API/Impl interface split.** ViaVersion splits every core type into an interface (in its
   `api/` module) and an `Impl` (in `common/`), such as `ProtocolManager` +
   `ProtocolManagerImpl`. endweave has no third-party API surface, so that split is deliberately
   collapsed into one concrete class per type. Reproducing it is over-engineering. The `@see` on
   the class names both halves.

4. **Every class, method, and member carries a `@see` to its upstream counterpart** (or a
   `@note endweave-specific.` if it has none). This is what makes the port maintainable. Reference
   upstream by its Java name, `#method` for members, e.g. `@see ViaVersion AbstractProtocol#registerServerbound`.

### The one deliberate divergence: node-per-version, not protocol-per-pair

ViaVersion has a protocol per adjacent-version *pair* (`Protocol1_20To1_20_2`), making a protocol
an **edge**. endweave has one per *version*, so `Protocol<V>` is a **node** that owns both
directions of the wire diff between V and the version registered before it. Consequences:

- A node's two handler tables are keyed by `Step { Upgrade, Downgrade }`, not clientbound and
  serverbound. `Protocol<V>::registerUpgrade` is the ViaVersion forward's `registerServerbound`,
  and `registerDowngrade` is the ViaBackwards backward's `registerClientbound`.
- `Protocol<V>` fuses a ViaVersion forward protocol with its ViaBackwards backward protocol.
- A node's own version is the *higher* end of its edge (ViaVersion's `clientVersion`), and its
  predecessor is the *lower* end (`serverVersion`).

This divergence is settled. Keep it, and keep documenting the mapping. Do not reintroduce
pair-named classes.

### Dropped as Java-Edition-specific (keep them dropped)

The `State` machine (Bedrock has one flat id space), the per-version `PacketType` enums and
name-based auto-mapping (Bedrock ids are stable and never renumbered, so the id *is* the
identity), `AbstractProtocol`'s four generic parameters, `MappingData`, the rewriter hierarchy,
and ViaVersion's `PacketWrapper` + `Type<T>` value registry.

### Loggers

There is one logger: the Endstone plugin logger, threaded through `ConnectionManager` and
`UserConnection`. It is the analogue of ViaVersion's `Via.getPlatform().getLogger()` global, held
rather than reached as a global. Match ViaVersion's levels: `info` for connection and lifecycle
lines, `warning` for problems and remap failures (ViaVersion's `AbstractProtocol#printRemapError`
logs at `WARNING`, and `SEVERE`/`error` is reserved for mapping-data load failures, which endweave
has none of). Do not add a per-protocol `ProtocolLogger` unless an upstream protocol actually logs
through one.

### Handlers speak decoded packets, never streams

A handler is a **converter between two eras' structs of one packet**, not a reader over a buffer.
It is registered by handing `registerUpgrade` / `registerDowngrade` / `registerServerbound` /
`registerClientbound` a function, and the packet id comes from the struct's `Id` rather than an
argument:

```cpp
std::expected<bp::FooPacket_<1001>, PacketError> upgradeFoo(UserConnection &connection,
                                                            const bp::FooPacket_<975> &packet);
...
registerUpgrade(&upgradeFoo);
```

`BinaryReader` and `BinaryWriter` are the codec's business, and no handler names them.

**Where ViaVersion throws, endweave returns `std::unexpected`.** Its handler is `void` and
reports everything through exceptions, so both of them land in the error channel here:
`CancelException` is `PacketError::Cancelled`, and an `InformativeException` is whichever
`PacketError` fits. The error type is endweave's own enum in `protocol/error.h`, not
`std::error_code`, and `describe()` is what the log line prints, which is ViaVersion's
`printRemapError`.

`ProtocolPipeline::transform` decodes once and encodes once: `PacketHolder` carries the body
undecoded until the first stage that handles the id asks for it as a type, every later stage
receives the struct the previous one left behind, and the body is written back out only if some
stage decoded it. A packet no stage handles is forwarded byte for byte and never decoded.

Two invariants hold the model up:

- A step whose two versions share a packet's shape needs no converter, because `Packet_<975>` and
  `Packet_<1001>` are then the same C++ type. A step that reshapes one and covers it nowhere is a
  **compile error**, not a runtime surprise. See "Coverage is checked at compile time" below.
- A body that decodes but leaves bytes unread is `PacketError::TrailingBytes`, so a schema that
  has drifted from the wire fails loudly instead of re-encoding a truncated packet.

`PacketHandlers` keeps `cancel()` and `then()`. ViaVersion's field converters (`map<From, To>`,
`create`, `read`, the `ValueReader` / `ValueWriter` / `ValueTransformer` family) have no
counterpart and must not be added back: a whole-struct converter is what replaced them.
`passthrough()` is gone for the same reason, since a packet nothing handles already passes
through.

**Packet ids come from bedrock-protocol.** `bp::MinecraftPacketIds` is the generated mirror of
BDS's enum, and endweave keeps no list of its own.

### Coverage is checked at compile time

In ViaVersion "no remapper" genuinely means "no difference", because a remapper is the only place
a difference can be written down. Here the codec knows the shapes independently, so a reshaped
packet nobody converted would be forwarded silently. A node therefore *declares* its coverage as
`using Upgrades` / `using Downgrades`, both optional, and `VersionNode` reads it back. The README
shows the shape.

- An entry is a converter (id from `In::Id`, never written twice), `cancel<Ids...>`, or
  `unconverted<Ids...>`. `Mappings<...>::And<...>` extends a list, since a reshape needs covering
  both ways and only the asymmetric packets differ between the two steps.
- `VersionNode::registerPackets()` is `final`. It pairs `bp::Registry<prev>::types` against
  `bp::Registry<cur>::types` by id and demands a declaration wherever `std::is_same_v` fails or a
  packet exists on one side only, then fills the tables from the same lists so the two cannot
  drift. Anything amending an already-covered id goes in an optional `registerExtras()`.
- A gap instantiates `MissingUpgradeConverterFor<P>` / `MissingDowngradeConverterFor<P>`, declared
  and never defined, so the build stops and names the packet type.

`unconverted<Ids...>` is not a loophole. It is the greppable admission the compiler forces someone
to write, and turning one into a converter is exactly what the translation work is.

**Only the base protocol translates so far.** Every entry in `Protocol<v26_30>` is `unconverted`
or `cancel`, so every packet still crosses the version step untouched.

### Two filters keep the cost off the hot path

Endstone dispatches every packet in both directions to every listener with no subscription
mechanism, so the listener is the hot path.

- `ProtocolManager::isInteresting(id)` is a `std::bitset<kPacketIdCount>` union of every registered
  protocol's tables, tested before the connection is looked up, so an id nothing registered costs
  no `SocketAddress` copy.
- `ProtocolPipeline::handles(direction, id)` is the same union over one connection's stages, built
  in `rebuild()`, replacing the scan over every pipe inside `transform()`.

An uninteresting packet therefore never reaches `UserConnection::touch()`, so the idle sweep only
evicts connections that never got through the handshake. Established ones go on `onDisconnect` or
`onPlayerQuit`, which is what ViaVersion does with the netty channel-close future.

## Correspondence map

| endweave | ViaVersion (api / common), unless noted |
| --- | --- |
| `protocol/protocol.h` `AbstractProtocol` | `Protocol` + `AbstractProtocol` |
| `protocol/protocol.h` `Protocol<V>` | a ViaVersion `ProtocolXToY` fused with the ViaBackwards `ProtocolYToX` |
| `protocol/version_node.h` `VersionNode` | *(none, the compile-time coverage check)* |
| `protocol/version_node.h` `Mappings` / `cancel` / `unconverted` | the `registerX` / `cancelX` calls in `registerPackets()`, declared rather than run |
| `protocol/manager.h` `ProtocolManager` | `ProtocolManager` + `ProtocolManagerImpl` |
| `protocol/pipeline.h` `ProtocolPipeline` | `ProtocolPipeline` + `ProtocolPipelineImpl` |
| `protocol/path.h` `ProtocolPathEntry` | `ProtocolPathEntry` + `ProtocolPathEntryImpl` |
| `protocol/path.h` `ProtocolPathKey` | `ProtocolPathKey` + `ProtocolPathKeyImpl` |
| `protocol/direction.h` `Direction` | `Direction` |
| `protocol/direction.h` `Step` | *(none, the node-model axis)* |
| `protocol/handler.h` `PacketHandler` / `PacketHandlers` | `PacketHandler` / `PacketHandlers` (remapper) |
| `protocol/handler.h` `PacketConverter` | *(none, the struct-to-struct handler signature)* |
| `protocol/error.h` `PacketError` | `CancelException` + `InformativeException`, returned rather than thrown |
| `protocol/packet.h` `PacketHolder` | what `PacketWrapper` carries, over one decoded struct |
| `bp::MinecraftPacketIds` (generated) | per-version `ClientboundPacketType`/`ServerboundPacketType`, flattened. Mirrors BDS |
| `connection/connection.h` `UserConnection` | `UserConnection` + `UserConnectionImpl` |
| `connection/connection.h` `ProtocolInfo` | `ProtocolInfo` + `ProtocolInfoImpl` |
| `connection/manager.h` `ConnectionManager` | `ConnectionManager` + `ConnectionManagerImpl` |
| `protocols/base/protocol.h` `InitialBaseProtocol` | `InitialBaseProtocol` |
| `plugin.{h,cpp}`, `listener.{h,cpp}` | platform module (plugin main + netty decode/encode handlers) |

Upstream lives under `com.viaversion.viaversion` (api in the `api/` module, impls in `common/`)
and `com.viaversion.viabackwards`.

## Adding a version

1. Add the enumerator to `ProtocolVersion` and append it to `kProtocolVersions`, both in
   `protocol/version.h`, in ascending order.
2. Add `src/endweave/protocols/vN/protocol.h` declaring `Protocol<ProtocolVersion::VN>` over
   `VersionNode`, with empty `Upgrades` and `Downgrades`.
3. Append one `registerProtocol<ProtocolVersion::VN>()` line to `registerProtocols()`, in ascending
   version order.
4. Build. Every packet the new edge reshapes is now a named compile error. Work through them,
   adding a converter, a `cancel<Id>`, or an `unconverted<Id>` for each. Converters go in a sibling
   `protocol.cpp`.

## Building

Requires CMake, Ninja, and Clang 18+ with libc++ (Endstone's toolchain floor). bedrock-protocol is
consumed from a sibling `../bedrock-protocol` checkout, and the Endstone SDK is fetched.

```shell
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake --build build
```

The plugin lands at `build/endstone_endweave.so`. Drop it in the server's `plugins/`.

## Code Style

- C++23, clang-format (see `.clang-format`). Classes/enums `CamelCase`, methods `camelBack`,
  private members `lower_case_` (trailing underscore), locals/params `lower_case`.
- Prefer explicit `.value()` on `std::optional` and `std::expected` over `operator*`. Check for
  presence first (`if (!x)`), then read through `.value()`.
- Prefer `std::unique_ptr` over `std::optional` to hold an owned object with deferred
  construction. Reserve `std::optional` for genuine value-presence (a missing `int`, an
  unreachable path, a cancelled body).
- **Comments:** terse and human. Default to no comment. When one is warranted, keep it to one
  short line. No multi-line explanations, rationale, design-decision narration, or parenthetical
  asides. No "LLM notes" restating what the code plainly does.
- **No LLM punctuation tells in prose.** Do not use em-dashes or semicolons in comments or
  docstrings. Use a period or a comma and rephrase.
- The `@see` and `@note endweave-specific.` correspondence lines (rule 4) are the deliberate
  exception to "default to no comment". They are required, and they are one line each.
