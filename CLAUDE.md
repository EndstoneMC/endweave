# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

endweave is a protocol-translation plugin for [Endstone](https://github.com/EndstoneMC/endstone).
It lets a Minecraft: Bedrock client on an older (or newer) protocol join a server running a
different one by translating packets on the wire. The wire codec comes from the sibling
[bedrock-protocol](https://github.com/EndstoneMC/bedrock-protocol) library. endweave owns the
translation semantics.

## Current state

The platform binding is in place: the plugin main, the packet listener, and the address-keyed
connection table. On top of it sit the `Transformer` specializations, which convert a decoded
struct of one era into the next era's. `InventoryContentPacket` and `StartGamePacket` translate
both ways between 1001 and 2168, along with every type they reach.

`protocol/handler.h` is the dispatch layer. It resolves a (from, to) version pair into a
`PacketHandlers` table and answers `get(id)` with the function that translates that packet, or null
where nothing has to happen.

Nothing includes it, so it is absent from the build. That matters: six packets reshape between 1001
and 2168 with no `Transformer` — ids 30, 32, 123, 315, 347 and 348 — and instantiating the tables
does not compile until they are written. `cmake --build` passes today only because no translation
unit reaches `handler.h`.

The remaining work is the wiring: read the client's version off `RequestNetworkSettingsPacket`,
resolve both tables onto `UserConnection`, and call them from the listener. `UserConnection` still
carries the type-keyed store that stateful handlers key into, and the listener still keeps the
connection table warm and evicts on disconnect and quit, so the table is live for it to hang off.

## What endweave takes from ViaVersion, and what it does not

endweave began as a port of [ViaVersion](https://github.com/ViaVersion/ViaVersion) (its forward
protocols) fused with [ViaBackwards](https://github.com/ViaVersion/ViaBackwards) (its backward
protocols), and is no longer one. Upstream registers handlers by hand at runtime; endweave derives
them from bedrock-protocol's schema at compile time. That single difference reaches everything above
the transforms — there is no `Protocol` class, no registration call, no `PacketWrapper`, no
`ProtocolPipeline`, no `ProtocolManager` and no path-finding. Do not restore any of them for the
sake of the resemblance. Structural fidelity was the earlier goal and is not the goal now; the
guarantees in **The handler layer** are, and several of them are only reachable by giving that
structure up.

Upstream stays valuable for three things.

- **Translation semantics.** ViaVersion and ViaBackwards encode years of per-packet knowledge: which
  field moved where, what a field with no source should default to, which rewrite is load-bearing
  and which is cosmetic. That is the part to read before writing a `Transformer`, it does not go
  stale, and nothing in this repo replaces it.
- **Vocabulary.** Where a type has a real counterpart it keeps upstream's name, so a reader can find
  the class it came from. See the correspondence map, and note that some names are ViaVersion's
  while the behaviour is Velocity's.
- **Problem decomposition.** The shape of the problem — per-packet handlers, per-version tables,
  connection-scoped resolution, and translate / passthrough / cancel as the three outcomes — is
  upstream's and is worth keeping.

### Rules

1. **Keep upstream's name where a counterpart exists, and only then.** A type that corresponds to a
   ViaVersion or Velocity one takes its name and carries a single `@see` line. A type with no
   counterpart gets the name that describes it and no `@see` at all; do not invent a correspondence
   to justify a name. Snake_case with a trailing underscore is the only spelling concession, so
   `serverProtocolVersion` becomes `server_protocol_version_`.

2. **Derive, do not register.** Anything that can be computed from the schema must be, and a helper
   that exists to do the computing needs no upstream precedent. `shouldHandle`, `shouldCancel`,
   `packet_of` and `ProtocolVersions::visit` have no ViaVersion counterpart and are the point. What
   still needs justifying is the opposite: a hand-maintained list, a runtime registration, or a
   lookup that could have been a compile-time one.

3. **A mistake that compiles is a design bug.** ViaVersion cannot tell you at build time that a
   packet was forgotten, because registration is a runtime call. Here it can, and the missing
   `Transformer` build failure is the whole return on abandoning the port. Weigh a change by what it
   still catches.

4. **No API/Impl interface split.** ViaVersion splits every core type into an interface (in its
   `api/` module) and an `Impl` (in `common/`), such as `ProtocolManager` + `ProtocolManagerImpl`.
   endweave has no third-party API surface, so that split is deliberately collapsed into one
   concrete class per type. Reproducing it is over-engineering. The `@see` names both halves.

Upstream lives under `com.viaversion.viaversion` (api in the `api/` module, impls in `common/`)
and `com.viaversion.viabackwards`.

## Settled decisions for the translation layer

Most of these are now realised in `protocol/handler.h` rather than pending; the logger rule is
still a directive. Either way they are binding.

- **No protocol classes at all.** ViaVersion has a protocol per adjacent-version *pair*
  (`Protocol1_20To1_20_2`) that packets are registered into by hand. endweave derives the whole
  thing instead: `handler.h` generates a handler per (from, to, id) out of bedrock-protocol's
  `packet_of`, so there is no node, no edge, and nothing to register. Do not reintroduce pair-named
  classes, and do not add a per-version roster — see **The handler layer**.
- **Transforms speak decoded packets, never streams.** A `Transformer` is a converter between two
  eras' structs of one packet, not a reader over a buffer, and none of them names `BinaryReader` or
  `BinaryWriter`. The generated `handle` is the one exception and is not a transform: it is the
  codec glue that decodes, calls one transform, and re-encodes. Keep the streams there.
- **Where ViaVersion throws, endweave returns `std::unexpected`.** Its handlers report everything
  through exceptions, and those land in the error channel here.
- **Packets come from bedrock-protocol, ids and types both.** `MinecraftPacketIds` is the generated
  mirror of BDS's enum and `packet_of<V, Id>` maps an id to the struct that carries it at that
  version. endweave keeps no list of its own, so a list can never go stale against the schema.
- **Dropped as Java-Edition-specific, keep them dropped:** the `State` machine (Bedrock has one flat
  id space), the per-version `PacketType` enums and name-based auto-mapping (Bedrock ids are stable
  and never renumbered, so the id *is* the identity), `AbstractProtocol`'s four generic parameters,
  `MappingData`, the rewriter hierarchy, and ViaVersion's `PacketWrapper` + `Type<T>` value registry.
- **One logger,** the Endstone plugin logger, threaded through rather than reached as a global. It
  is the analogue of ViaVersion's `Via.getPlatform().getLogger()`. Match ViaVersion's levels: `info`
  for connection and lifecycle lines, `warning` for problems and remap failures.

## The handler layer

`protocol/handler.h` turns a runtime (version, version, packet id) into a typed call, and does the
whole mapping at compile time. Nothing is registered, so nothing can be forgotten: the packets that
need work are derived from the schema, and one that needs work but has no `Transformer` fails to
compile.

- **A handler is generated, not registered.** `handle<From, To, Id>` deserializes `packet_of<From,
  Id>`, calls one transform, and serializes the result. `handlerFor` returns its address where the
  packet moves between the two versions and `nullptr` where it does not, and `makeHandlers` lays
  those out into a `constexpr` array indexed by id. This is the whole of what ViaVersion does with
  `registerClientbound` calls in a protocol constructor.
- **A missing `Transformer` is a build error, and that is the release gate.** ViaVersion's
  registration is a runtime call, so forgetting a packet still compiles, still starts, and quietly
  hands a client bytes it cannot parse. Here the id sweep reaches every packet the schema models, so
  a reshaped packet with no transform stops the build. Do not add an escape hatch that lets an
  unported packet fall through to passthrough — the silence is the failure this design exists to
  prevent. The build failing *is* the answer to "can we ship yet".
- **Type identity is the wire diff.** `shouldHandle` is `has_packet` at both ends and
  `!is_same_v<packet_of<From, Id>, packet_of<To, Id>>`. bedrock-protocol emits one type per distinct
  shape and propagates versioning transitively — `StartGamePacket` is versioned at 1001 only because
  `LevelSettings` moved underneath it — so the same type means the same bytes, and an unchanged
  packet keeps its payload rather than round-tripping through a codec for nothing.
- **A packet the destination does not have is cancelled, never forwarded.** `shouldCancel` is
  `has_packet<From, Id> && !has_packet<To, Id>`, and that one predicate covers every case: a newer
  client sending something an older server never knew, a newer server sending something an older
  client cannot parse, and either direction across a version that removed a packet. `isCancelled(id)`
  is a second `constexpr` table beside the handlers, queried before them, so a cancelled packet costs
  one indexed load and builds no buffer either. Cancelling and handling are disjoint by
  construction, since `shouldHandle` demands the packet at both ends. Forwarding an id the peer does
  not know is worse than dropping it — it desynchronises or disconnects.
- **Cancellation reaches only as far as the schema does.** `has_packet` means "bedrock-protocol
  models this", not "this exists on the wire", so an id modelled at neither version reads false on
  both sides and still passes through: nothing in the schema says whether the destination has it.
  Between 1001 and 2168 nothing cancels today, because both model the same 18 packets. The predicate
  first bites at a real range boundary — `ClientboundUpdateSoundDataPacket` (348) arrives at 1001,
  so 1001 to 975 cancels it.
- **Null means passthrough, and the caller must be able to see it before it builds anything.**
  `PacketHandler` takes only the two streams, so `PacketHandlers::get(id)` answers without them. A
  handler that takes the id would force the caller to construct a `BinaryReader` and a
  `BinaryWriter` just to learn there was nothing to do. Keep the id lookup free of buffers.
- **Passthrough is the null handler, not a return value.** A handler that runs either succeeds or
  carries an `error_code`, which is why it returns `std::expected<void, std::error_code>`. Do not
  reintroduce a result enum with a `Passthrough` case.
- **Resolve once per connection, never per packet.** `getPacketHandlers(from, to)` is the analogue
  of Velocity's `getProtocolRegistry`, which the decoder caches rather than re-looking-up. Store both
  directions on `UserConnection`; per packet it is then a bounds check and an indexed load.
- **`kHandlers` must keep static storage.** `PacketHandlers` holds a `std::span` into it. Build the
  array inline in the constructor call instead and the span points at a temporary — constant
  evaluation catches it, but a runtime resolve would compile and read freed stack.
- **The runtime-to-compile-time crossing happens twice, and only twice.**
  `ProtocolVersions::visit` folds a runtime `ProtocolVersion` into a template argument; the id is a
  plain array index. Everything below is monomorphic.
- **Two versions is one hop.** The `Transformer<From, To>` key carries the direction, so `handle`
  names the destination outright and nothing branches on `From < To`. A chained walk across
  intermediate nodes is the ViaVersion shape and comes back when a third version does; `next` and
  `prev` are parked in `version.h` for it and have no callers today.
- **`ProtocolVersion::UNKNOWN` is the sentinel, not `std::optional`.** It matches Velocity's
  `getProtocolVersion(int)`, and it composes: an unknown version matches no fold arm, so the table
  comes back empty and every `get(id)` is null without a presence check at any call site.
- **Ids 200-299 are skipped** as the vendor extension range, off `MinecraftPacketIds`' own
  `TITLE_SPECIFIC_PACKETS_START` / `_END` sentinels rather than literals.

One known hole: `is_same_v` over-approximates. The compiler can materialise a snapshot whose
serializer is byte-identical to the previous one, giving two C++ types that encode the same wire —
`LevelSoundEventPacket` (123) is the live case, since it stopped carrying the `LevelSoundEvent` enum
at 1001 and a name-coded string cannot be perturbed by enumerators added at 2168. That belongs in
bedrock-protocol, which should alias rather than re-emit an identical snapshot, not in a
`wire_equal_v` workaround here.

## Transformers

`Transformer` is the analogue of ViaVersion's `ValueTransformer`, in the shape of `std::formatter`:
a trait declared undefined in `protocol/transform.h` and specialized per source/destination pair
that changes shape. That header also holds the `ew::transform` / `ew::transform_to` call surface and
the `std::optional` / `std::vector` / `std::map` specializations. A transform returns its result
outright — the `std::unexpected` channel above has no bearing on it yet, which is what the last rule
here is about.

- **Keyed on the source and destination pair, with one method.**
  `Transformer<v1001::Foo, v2168::Foo>::transform` returns a `v2168::Foo` and the opposite pair
  returns the 1001 one, so the key names both ends and the method never has to. The pair is what
  keeps the key unique once a third era exists: an era below adds
  `Transformer<v1001::Foo, v975::Foo>`, a distinct specialization rather than a competing one. One
  source reaching several destinations is the ordinary case here, not a special one. A type that
  did not change between two eras is one C++ type in both namespaces and needs no specialization.
- **One file per source version,** `protocols/<version>/`, holding every specialization whose source
  type belongs to that version. v1001 and v2168 are currently the outermost eras modelled, so v1001
  holds the pairs leaving 1001 and v2168 the pairs leaving 2168; a missing pair is a "no
  `Transformer<From, To>`" error rather than a wrong conversion.
- **`Transformable<From, To>` is the availability test,** and it asks whether a call to
  `Transformer<From, To>::transform` is well-formed and returns exactly `To` — not whether the
  specialization exists. A declaration alone never reads as a supported edge, and a container pair
  whose elements have no transform reports as untransformable rather than as a body that happens to
  fail later.
- **A type with two wire shapes at one version is just another pair.** BDS writes
  `SerializedSkinRef` two ways at 1001: cerealised for `PlayerSkinPacket`, and through
  `SerializedSkinImpl::write` for a `PlayerListPacket` entry, which bedrock-protocol emits as
  `bp::legacy::SerializedSkinRef`. That is not a version hop, and it needs no second axis either:
  `bp::legacy::SerializedSkinRef` is a distinct C++ type, so
  `Transformer<bp::SerializedSkinRef_<1001>, bp::legacy::SerializedSkinRef>` sits beside
  `Transformer<bp::SerializedSkinRef_<1001>, bp::SerializedSkinRef_<2168>>` without colliding. Both
  keys are 1001-era, so both live in `protocols/v1001/skin.{h,cpp}` — the module that owns the
  types. A conversion never earns a file or a free function of its own. A chain has to name its
  middle type, since a proxy cannot feed another `transform`:
  `ew::transform(ew::transform_to<bp::SerializedSkinRef_<1001>>(std::move(entry.skin)))`.
- **The specialization is declared in the header, the body defined in the sibling `.cpp`,** which is
  listed in `endstone_add_plugin`. An out-of-line body rules out a deduced return type, so the
  declaration spells the returned struct outright. No trailing return types. The destination now
  also appears in the key, which makes the return type on the definition look redundant — it is not
  droppable: an out-of-line definition must repeat the declared return type, and `auto` on both ends
  would only deduce within the defining `.cpp`, leaving every other translation unit unable to call
  it.
- **A transform consumes its source.** It takes an rvalue reference and moves every field that owns
  storage, since the packet it came from is on its way out. A `const` source is a compile error
  rather than a copy.
- **Assign every field explicitly, in declaration order.** Only a field whose shape actually changed
  carries logic, and it reads as the odd one out against the plain assignments around it.
- **A packet delegates to the `Transformer` of each changed field's type,** moving into it. The
  arithmetic of a changed field lives in that field's transform, never restated at the packet.
- **Call through `ew::transform`,** which returns a proxy that takes its destination from the
  assignment target, so a field reads `to.slots = ew::transform(std::move(from.slots));` — no
  versioned type at the call site. The `std::move` is not optional: `transform` preserves the value
  category it is handed and every body takes `&&`, so an lvalue is a compile error rather than a
  silent copy. It always consumes what it is handed, so never pass it something the rest of the body
  still reads, and hoist any member read out of an argument list that also moves the object.
  Where there is no destination context — an `auto` local, the inner half of a chain — use
  `ew::transform_to<Dest>(std::move(x))` and spell the destination. Qualifying is not optional:
  inside a `Transformer<...>::transform` body the unqualified name finds the member, lookup stops at
  class scope, and the free function is never a candidate.
- **`std::optional`, `std::vector` and `std::map` are already specialized,** each pairing the
  container of the source element with the container of the destination one. They unwrap, delegate
  to the element's `Transformer`, and compose (`optional<vector<T>>`), so a field never spells a
  loop or a `has_value()` guard. An element with no `Transformer` is a compile error, which is what
  keeps a missing include from passing the value through untranslated.
- **A projection is written out both ways.** v1001's tagged `ItemStackNetIdVariant` reaches v2168 as
  one signed varint (`n` for an `ItemStackNetId`, `-2n-1` for an `ItemStackRequestId`, `-2n` for an
  `ItemStackLegacyRequestId`), and v2168's transform reads the case back from sign and parity. The
  pair round-trips.
- **Establish what changed by static-asserting `is_same_v` on every field pair,** not by reading the
  two structs side by side. Of `StartGamePacket`'s 26 fields only four move, and one of them,
  `ServerConfigurationJoinInfo`, is versioned in a module the packet's own namespace gives no sign
  of. `LevelSettings` changes 2 of 49.
- **A field can migrate into a nested type rather than be renamed.** v1001's loose
  `experiments_previously_toggled` is v2168's `experiments.experiments_ever_toggled`, so a name that
  vanishes from the top level is worth hunting for one level down before treating it as dropped.
- **A field with no source is invented, and says so.** Downgrading writes `is_chat_logging = false`,
  and `value_or({})` where 2168 made a field optional that 1001 required, so an absent world id
  becomes a null UUID. `PresenceConfiguration`'s `experience_name` and `world_name` are simply gone
  at 2168 and come back `std::nullopt`. Each of these is a candidate to refuse the downgrade once
  there is an error channel; none should be quietly widened into looking faithful.

## Correspondence map

| endweave | ViaVersion (api / common), unless noted |
| --- | --- |
| `connection/connection.h` `UserConnection` | `UserConnection` + `UserConnectionImpl` |
| `connection/manager.h` `ConnectionManager` | `ConnectionManager` + `ConnectionManagerImpl` |
| `plugin.{h,cpp}`, `listener.{h,cpp}` | platform module (plugin main + netty decode/encode handlers) |
| `protocol/transform.h` `Transformer` | `ValueTransformer` |
| `protocol/handler.h` `PacketHandler` | `PacketHandler` (`api/protocol/remapper/`) |
| `protocol/handler.h` `PacketHandlers` | `PacketHandlers` by name, Velocity `ProtocolRegistry` by behaviour |
| `protocol/handler.h` `getPacketHandlers` | Velocity `StateRegistry.PacketRegistry#getProtocolRegistry` |
| `protocol/version.h` `ProtocolVersion` | `ProtocolVersion` |
| `protocol/version.h` `ProtocolVersions` | Velocity `ProtocolVersion#SUPPORTED_VERSIONS` / `#getProtocolVersion(int)` |

## Building

Requires CMake, Ninja, and Clang 18+ with libc++ (Endstone's toolchain floor). bedrock-protocol is
consumed from a sibling `../bedrock-protocol` checkout, and the Endstone SDK is fetched.

```shell
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake --build build
```

The plugin lands at `build/endstone_endweave.so`. Drop it in the server's `plugins/`.

There is no test suite. `CMakeLists.txt` still guards `tests/` behind `ENDWEAVE_BUILD_TESTS`, but
nothing defines that option and `tests/CMakeLists.txt` is empty: bedrock-protocol's own goldens
cover the codec, and a transform is exercised by the packets that run through it.

A green build is not yet evidence that the handlers compile, because no source file includes
`handler.h`. The gate only fires once a translation unit reaches it — which the listener will do
when it starts calling `getPacketHandlers`, and which a one-line `#include` would do sooner.

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
  clashes with `winnt.h` once `<endstone/endstone.hpp>` has pulled in `windows.h`. `handler.h` is
  the exception and must take the umbrella: `bpc` compiles one module at a time, so a lone module
  include leaves every other module's packets reading as unmodelled. Keep `handler.h` out of any
  translation unit that also includes `<endstone/endstone.hpp>`, or that clash lands on Windows.
- `namespace bp = bedrock::protocol;`, declared after the includes and above `namespace endweave`.
  Generated types are spelled through it, so a versioned one reads `bp::v1001::Foo`. `namespace ew =
  endweave;` follows the same placement, but only in the `.cpp` that calls through it — an alias for
  our own namespace has no business in a header everything includes.
