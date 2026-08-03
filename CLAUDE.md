# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

endweave is a protocol-translation plugin for [Endstone](https://github.com/EndstoneMC/endstone).
It lets a Minecraft: Bedrock client on an older (or newer) protocol join a server running a
different one by translating packets on the wire. The wire codec comes from the sibling
[bedrock-protocol](https://github.com/EndstoneMC/bedrock-protocol) library. endweave owns the
translation semantics.

## Current state

The translation layer has been removed. What is left is the platform binding: the plugin main, the
packet listener, and the address-keyed connection table. Nothing is translated, and no protocol,
pipeline, or version node exists. Rebuilding the translation layer is the work, and the
`Transformer` specializations for `SerializedNetworkItemStackDescriptor` are its first piece.

`UserConnection` still carries the type-keyed store that stateful handlers key into, and the
listener still keeps the connection table warm and evicts on disconnect and quit, so the table is
live for translation to hang off.

## The prime directive: endweave is an exact port of ViaVersion

endweave is a faithful C++ port of [ViaVersion](https://github.com/ViaVersion/ViaVersion) (its
forward protocols) fused with [ViaBackwards](https://github.com/ViaVersion/ViaBackwards) (its
backward protocols). Maintainability against upstream is the point. A dev looking at any endweave
file should be able to find the ViaVersion class it came from, so that when upstream changes, the
corresponding endweave code can be updated in lockstep.

Treat the ViaVersion Java source as the specification. When in doubt, match it.

### Rules

1. **Mirror upstream structure, names, hierarchy, and members.** Classes keep their ViaVersion
   names, the inheritance tree matches, methods keep their upstream names and semantics, and member
   fields correspond one to one. Snake_case with a trailing underscore is the only concession, so
   `serverProtocolVersion` becomes `server_protocol_version_`.

2. **Do not add methods, helpers, or members that are not in upstream ViaVersion.** If you reach
   for something ViaVersion does not have, it must be one of:
   - **Platform glue**, the Endstone integration ViaVersion keeps in its own platform modules
     (the plugin main, the netty handlers, connection lifecycle). Lives in `plugin.*`,
     `listener.*`, and the address-keyed connection bookkeeping.
   - **A language-gap accommodation**, something the JVM gives ViaVersion for free that C++ must
     spell out, such as ownership where Java relies on GC.

3. **No API/Impl interface split.** ViaVersion splits every core type into an interface (in its
   `api/` module) and an `Impl` (in `common/`), such as `ProtocolManager` + `ProtocolManagerImpl`.
   endweave has no third-party API surface, so that split is deliberately collapsed into one
   concrete class per type. Reproducing it is over-engineering. The `@see` names both halves.

Upstream lives under `com.viaversion.viaversion` (api in the `api/` module, impls in `common/`)
and `com.viaversion.viabackwards`.

## Settled decisions for the rebuilt translation layer

These are directives, not descriptions of code that exists today.

- **Node per version, not protocol per pair.** ViaVersion has a protocol per adjacent-version
  *pair* (`Protocol1_20To1_20_2`), making a protocol an **edge**. endweave has one per *version*, so
  a protocol is a **node** owning both directions of the wire diff between its version and the one
  registered before it. A node's handler tables are keyed by an upgrade/downgrade step, not by
  clientbound and serverbound, and a node fuses a ViaVersion forward protocol with its ViaBackwards
  backward protocol. Do not reintroduce pair-named classes.
- **Handlers speak decoded packets, never streams.** A handler is a converter between two eras'
  structs of one packet, not a reader over a buffer. The packet id comes from the struct rather than
  an argument, and no handler names `BinaryReader` or `BinaryWriter`.
- **Where ViaVersion throws, endweave returns `std::unexpected`.** Its handlers report everything
  through exceptions, and those land in the error channel here.
- **Packet ids come from bedrock-protocol.** `bedrock::protocol::MinecraftPacketIds` is the
  generated mirror of BDS's enum, and endweave keeps no list of its own.
- **Dropped as Java-Edition-specific, keep them dropped:** the `State` machine (Bedrock has one flat
  id space), the per-version `PacketType` enums and name-based auto-mapping (Bedrock ids are stable
  and never renumbered, so the id *is* the identity), `AbstractProtocol`'s four generic parameters,
  `MappingData`, the rewriter hierarchy, and ViaVersion's `PacketWrapper` + `Type<T>` value registry.
- **One logger,** the Endstone plugin logger, threaded through rather than reached as a global. It
  is the analogue of ViaVersion's `Via.getPlatform().getLogger()`. Match ViaVersion's levels: `info`
  for connection and lifecycle lines, `warning` for problems and remap failures.

## Transformers

`Transformer` is the analogue of ViaVersion's `ValueTransformer`, in the shape of `std::formatter`:
a trait declared undefined in `protocol/transform.h` and specialized per type that changes shape.

- **Keyed on the source type, and the target falls out of the return type.**
  `Transformer<v1001::Foo>::transform` returns a `v2168::Foo`, so the call site names only what it
  has. A type that did not change between two eras is one C++ type in both namespaces and needs no
  specialization.
- **One file per source version,** `protocols/<version>/transform.{h,cpp}`, holding every
  specialization whose source type belongs to that version. `protocols/v1001/transform.h` converts
  a v1001 struct into its v2168 counterpart, and `protocols/v2168/transform.h` the way back.
- **The specialization is declared in the header, the body defined in the sibling `.cpp`,** which is
  listed in `endstone_add_plugin`. An out-of-line body rules out a deduced return type, so the
  declaration spells the returned struct outright. No trailing return types.
- **A transform consumes its source.** It takes an rvalue reference and moves every field that owns
  storage, since the packet it came from is on its way out. An lvalue caller has to say `std::move`,
  so a copy is never silent.
- **Assign every field explicitly, in declaration order.** Only a field whose shape actually changed
  carries logic, and it reads as the odd one out against the plain assignments around it.
- **A projection is written out both ways.** v1001's tagged `ItemStackNetIdVariant` reaches v2168 as
  one signed varint (`n` for an `ItemStackNetId`, `-2n-1` for an `ItemStackRequestId`, `-2n` for an
  `ItemStackLegacyRequestId`), and v2168's transform reads the case back from sign and parity. The
  pair round-trips.

## Correspondence map

| endweave | ViaVersion (api / common), unless noted |
| --- | --- |
| `connection/connection.h` `UserConnection` | `UserConnection` + `UserConnectionImpl` |
| `connection/manager.h` `ConnectionManager` | `ConnectionManager` + `ConnectionManagerImpl` |
| `plugin.{h,cpp}`, `listener.{h,cpp}` | platform module (plugin main + netty decode/encode handlers) |
| `protocol/transform.h` `Transformer` | `ValueTransformer` |

## Building

Requires CMake, Ninja, and Clang 18+ with libc++ (Endstone's toolchain floor). bedrock-protocol is
consumed from a sibling `../bedrock-protocol` checkout, and the Endstone SDK is fetched.

```shell
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake --build build
```

The plugin lands at `build/endstone_endweave.so`. Drop it in the server's `plugins/`.

`ENDWEAVE_BUILD_TESTS` defaults to `ON` for a top-level build and pulls in `tests/`.

## Code Style

- C++23, clang-format (see `.clang-format`). Classes/enums `CamelCase`, methods `camelBack`,
  private members `lower_case_` (trailing underscore), locals/params `lower_case`.
- Prefer explicit `.value()` on `std::optional` and `std::expected` over `operator*`. Check for
  presence first (`if (!x)`), then read through `.value()`.
- Prefer `std::unique_ptr` over `std::optional` to hold an owned object with deferred
  construction. Reserve `std::optional` for genuine value-presence.
- **Comments: do not write any.** The one exception is the upstream correspondence line, a single
  `/** @see ViaVersion Foo#bar. */` on a type or method that has a ViaVersion counterpart. Nothing
  else, no explanations, no rationale, no `@param` or `@return` blocks, no trailing notes on
  members. Comments already in the tree that a human wrote stay.
- Include `<protocol/network.h>` and friends rather than the `<bedrock/protocol.hpp>` umbrella when
  only one module is needed. The umbrella's `protocol/game.h` has an enumerator named `VOID` that
  clashes with `winnt.h` once `<endstone/endstone.hpp>` has pulled in `windows.h`.
- `namespace bp = bedrock::protocol;`, declared after the includes and above `namespace endweave`.
  Generated types are spelled through it, so a versioned one reads `bp::v1001::Foo`.
