# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

endweave is a protocol-translation plugin for [Endstone](https://github.com/EndstoneMC/endstone).
It lets a Minecraft: Bedrock client on an older (or newer) protocol join a server running a
different one by translating packets on the wire. The wire codec comes from the sibling
[bedrock-protocol](https://github.com/EndstoneMC/bedrock-protocol) library. endweave owns the
translation semantics.

It ships as a Python wheel, `endstone-endweave`. The plugin is Python; the translation engine is a
C++23 extension, `endweave._pipeline`, built with nanobind against bedrock-protocol.

## Architecture

Two halves, and only bytes, ints and strings cross between them.

- **The plugin** (`src/endweave/*.py`) owns connections, the handshake, config, commands and
  logging. It is ViaVersion's platform and connection layer, ported to Endstone's packet events.
- **The engine** (`src/endweave/protocol/*.h`, `src/endweave/protocols/`, bound in
  `src/endweave/_pipeline.cpp`) owns the per-packet work. It names no Endstone type, so it shares
  nothing with Endstone's own pybind11 module.

### The plugin

`plugin.py` stands in for the netty pipeline. `on_packet_receive` runs at `LOWEST`, so other
plugins read a received packet as the server's version; `on_packet_send` runs at `HIGHEST` with
`ignore_cancelled`, so other plugins read a sent packet as the server's version before it is carried
to the client's, as ViaVersion's encoder sits below the server's own.

`protocol/base.py` `BaseProtocol` runs first on every received packet. On
`RequestNetworkSettings` (193) it reads the client's protocol off the first four bytes, installs a
`Translator` each way on the `Connection`, and writes the server's protocol number over the
client's; `Login` (1) gets the same rewrite, since BDS checks it again. Each connection gets
`_pipeline.Translator`s of its own, never shared or cached, since a translator keeps the blocks its
client was sent. A client on the server's own number, or one the engine does not carry, gets neither
pipeline nor rewrite: the server takes or refuses it itself. Where the two numbers name one wire
shape, as 26.45 and 26.40-26.44 do, the pipeline translates nothing and the rewrite is the whole of
the work. The blocked version gate runs on `PlayerLoginEvent`.

`connection.py` keys connections by the peer's address string, so a NetherNet peer (empty hostname)
is never tracked, and pending connections are capped at `MAX_PENDING_CONNECTIONS`.

Per packet, `EndweavePlugin._translate` looks the id up in the translator's `actions`:

- **absent:** leave the event alone. Do not assign the payload back: Endstone treats a new payload as
  a changed packet and rebuilds the frame.
- **`Action.CANCEL`:** cancel the event.
- **`Action.TRANSLATE`:** call `translate(packet_id, payload)`. `None` means a transform refused it,
  and the event is cancelled. `TranslationError` (with `packet_id` and `stage`) means it failed to
  decode, had bytes left over, or did not read back; the event is cancelled and the player kicked.

### Two version registries

- `protocol/version.py` is ViaVersion's `ProtocolVersion`: every Bedrock protocol endweave can
  name, with the Minecraft versions each covers. `/endweave list` and `block-versions` read it, and
  the README table mirrors it.
- `protocol/version.h` `ProtocolVersions::SUPPORTED_VERSIONS` is what the engine translates: 2168,
  2193 and 2225, a sorted line that `step(from, to)` routes along one hop at a time, so there is no path
  search. `WIRE_IDENTICAL` routes 2169 (26.45) as 2168, and `handler.h` static-asserts that
  claim against the schema, so a version that stops being identical fails the build.
- `getProtocolVersion` also routes a number one past a listed one as that one, unchecked, since
  Bedrock adds one for a release after its last preview (2192 to 2193 for 26.50) or for a hotfix,
  and the packets have mostly stayed the same. `BaseProtocol` warns the first time it sees each such
  client number, and `on_enable` warns for the server's. Once the release ships, give its number a
  `WIRE_IDENTICAL` entry or a `SUPPORTED_VERSIONS` one.

### The engine

`protocol/handler.h` turns a runtime (from, to, packet id) into a typed call and does the whole
mapping at compile time. `getTranslator(from, to)` folds the two runtime versions into template
arguments through `ProtocolVersions::visit` and returns a `Translator` over `kHandlers<From, To>`,
a `constexpr` array of `PacketHandler` indexed by id.

| header | holds | says |
| --- | --- | --- |
| `protocol/transform.h` | `Transformer<FromType, ToType>` | how a shape change is carried across one hop |
| `protocol/transform.h` | `memberwise_complete_v<FromType, ToType>` | the destination names every member the source does, so the copy writes itself |
| `protocol/rewrite.h` | `Rewriter<From, To, Id>` | this packet needs a value fixed that the wire diff cannot see |
| `protocol/context.h` | `Context<To>` | what a transform is handed: the cancel flag and the object to fill |

## What endweave takes from ViaVersion, and what it does not

[ViaVersion](https://github.com/ViaVersion/ViaVersion) (forward protocols) and
[ViaBackwards](https://github.com/ViaVersion/ViaBackwards) (backward protocols) register handlers by
hand at runtime; endweave derives them from bedrock-protocol's schema at compile time. That single
difference reaches everything above the transforms: there is no `Protocol` class, no registration
call, no `ProtocolPipeline`, no `ProtocolManager` and no path-finding. Do not add any of them for
the sake of the resemblance. The guarantees in **The handler layer** are the goal, and several of
them are only reachable without that structure.

Upstream stays valuable for three things.

- **Translation semantics.** ViaVersion and ViaBackwards encode years of per-packet knowledge: which
  field moved where, what a field with no source should default to, which rewrite is load-bearing
  and which is cosmetic. That is the part to read before writing a `Transformer`.
- **Vocabulary.** Where a type has a real counterpart it keeps upstream's name, so a reader can find
  the class it came from. See the correspondence map, and note that some names are ViaVersion's
  while the behaviour is Velocity's.
- **Problem decomposition.** Per-packet handlers, per-version-pair tables, connection-scoped
  resolution, and translate / passthrough / cancel as the three outcomes.

### Rules

1. **Keep upstream's name where a counterpart exists, and only then.** A type that corresponds to a
   ViaVersion or Velocity one takes its name and carries a single `@see` line (a `See Also:` block
   in a Python docstring). A type with no counterpart gets the name that describes it and no `@see`
   at all; do not invent a correspondence to justify a name.

2. **Derive, do not register.** Anything that can be computed from the schema must be, and a helper
   that exists to do the computing needs no upstream precedent. `should_translate_v`,
   `should_cancel_v`, `packet_of_t` and `ProtocolVersions::visit` have no ViaVersion counterpart and
   are the point. What still needs justifying is the opposite: a hand-maintained list, a runtime
   registration, or a lookup that could have been a compile-time one.

3. **A mistake that compiles is a design bug.** ViaVersion cannot tell you at build time that a
   packet was forgotten, because registration is a runtime call. Here it can, and the missing
   `Transformer` build failure is what the schema-derived design buys. Weigh a change by what it
   still catches.

4. **No API/Impl interface split.** ViaVersion splits core types into an interface (`api/`) and an
   `Impl` (`common/`). endweave has no third-party API surface, so each is one concrete class, and
   the `@see` names both halves.

5. **Leave out what only Java Edition needs.** The `State` machine (Bedrock has one flat id space),
   per-version `PacketType` enums and name-based auto-mapping (Bedrock ids are stable), the item,
   sound and entity halves of `MappingData`, the rewriter hierarchy, `Type<T>`, and the
   platform-split config options. Block states are the exception: every version renumbers them, so
   `getNewBlockStateId` carries the block half of `MappingData` (see **Block states**). Of `PacketWrapper`,
   only the cancel half exists, as `Context`; the buffer half has no place because the schema
   derives the codec. Of `UserConnection`, there is no `StorableObject` store: no transform keeps
   state across packets, so none is carried.

Upstream lives under `com.viaversion.viaversion` (api in the `api/` module, impls in `common/`)
and `com.viaversion.viabackwards`.

## The handler layer

- **A handler is generated, not registered.** `handle<From, To, Id>` deserializes
  `packet_of_t<From, Id>`, applies the `Rewriter`, walks the transforms, and serializes the result.
  `handlerFor` returns its address where the packet needs work, `&detail::cancel` where it is
  dropped, and `nullptr` where nothing happens; `makeHandlers` lays those out by id. This is the
  whole of what ViaVersion does with `registerClientbound` calls in a protocol constructor.
- **A missing `Transformer` is a build error, and that is the release gate.** `_pipeline.cpp`
  includes `handler.h`, and `getTranslator` instantiates `kHandlers` for every supported pair, so a
  reshaped packet with no transform stops the build. Do not add an escape hatch that lets an
  unported packet fall through to passthrough: the silence is the failure this design exists to
  prevent.
- **Type identity is the wire diff.** bedrock-protocol emits one type per distinct shape and
  propagates versioning transitively, so the same type means the same bytes.
  `should_translate_v` is true when `From != To`, the packet is modelled at every version on the
  path, and either a `Rewriter` applies or `packet_of_t<From, Id>` and `packet_of_t<To, Id>` differ.
- **A packet the destination does not have is cancelled, never forwarded.** `should_cancel_v` is
  `has_packet_v<From, Id>` with a gap anywhere on the path. That covers a newer client sending
  something an older server never knew and a newer server sending something an older client cannot
  parse. Forwarding an id the peer does not know desynchronises or disconnects it.
- **Cancellation reaches only as far as the schema does.** `has_packet_v` means "bedrock-protocol
  models this", not "this exists on the wire", so an id modelled at neither end passes through.
  `SetPlayerFurnaceOptions` (351) is modelled at 2193 alone, so 2193 towards 2168 cancels it.
- **Null means passthrough, and the caller sees it before building anything.** `Translator::get(id)`
  answers without a reader or writer, and the binding publishes the non-null entries as `actions`,
  so a passthrough packet never crosses into C++. Keep the id lookup free of buffers.
- **Passthrough is the null handler, not a return value.** A handler that runs either succeeds or
  carries an `error_code`, so it returns `std::expected<void, std::error_code>`. Cancel is a
  handler too, and `getAction` recognises it by address. Do not add a result enum.
- **Every translated packet is checked twice.** `handle` rejects leftover source bytes
  (`protocol_error`), since a schema that models part of a packet still deserializes, and reads its
  own output back at `To` (`bad_message`), since the serializer and deserializer are generated apart.
- **Resolve once per version pair, never per packet.** `Translator` is the analogue of Velocity's
  `ProtocolRegistry`, and the plugin caches one per pair. Per packet it is a dict lookup in Python
  and an indexed load in C++.
- **`kHandlers` must keep static storage.** `Translator` holds a `std::span` into it. Build the
  array inline in the constructor call instead and the span points at a temporary.
- **One handler per endpoint pair, a chain of transforms inside it.** `chain` walks the versions
  between the two ends one hop at a time: decode once at `From`, hand the struct through
  `Transformer<packet_of_t<Cur, Id>, packet_of_t<next, Id>>` for each hop that reshapes, encode once
  at `To`. There is no intermediate serialization. `Transformer` stays keyed on adjacent versions, so
  adding a version costs its neighbours' transforms and nothing quadratic.
- **A hop whose type did not change is skipped, not transformed.** `chain` passes the struct straight
  on when both sides are one type, so it needs no `Transformer<T, T>`. Do not add one: it would
  collide with the container partial specializations in `transform.h`.
- **A client already on the server's protocol is left alone.** `should_translate_v` is false when
  `From == To`, and `BaseProtocol` installs nothing for a client announcing the server's own number.
  A client whose different number names the same wire shape gets a pipeline with empty `actions`, so
  no packet of its own crosses into C++ either. The guard is load-bearing because a `Rewriter` can
  force translation on its own.
- **Ids 200-299 are skipped** as the vendor extension range, off `MinecraftPacketIds`'
  `TitleSpecificPacketsStart` / `TitleSpecificPacketsEnd` rather than literals.

## `Rewriter`: work that the wire diff cannot see

Some packets need attention whether or not their shape moved: a checksum the other side cannot
reproduce, an enumerator renumbered between versions. That is a semantic change, not a shape change,
and it is not `Transformer`'s job.

- **Keyed on the pair, applied once to the source.** `Rewriter<From, To, Id>::rewrite(packet_of_t<From,
  Id> &)` runs before the chain and writes `To`'s values into the `From` struct. Once rather than per
  hop, because a value both ends share may be missing from a version in between.
- **It forces translation.** A `Rewriter` makes `should_translate_v` true even when both ends are one
  type, and the packet is then re-encoded as is. Everything on the chain must carry the rewritten
  field across untouched, which is what a transform does with a field it does not reshape.
- **Pair-agnostic rewriters are partial specializations over `From` and `To`,** in
  `protocols/rewriters.h`: `StartGamePacket`'s block registry checksum, which is zeroed because no
  translation reproduces it, the heartbeat `LevelSoundEvent` inside actor data, and the block states
  the schema cannot type (see **Block states**). A rewriter for one pair goes in that pair's
  directory.
- **One that parses bytes the schema leaves opaque returns `std::expected<void, std::error_code>`,**
  and `handle` fails the packet on an error, as it does a decode error. The rest return `void`.
- **One that needs the blocks the client was sent takes the translator's `BlockConnectionStorage &`**
  as a second argument, and one that needs the items the client lacks its `ItemRegistryStorage &`.
  One that gives the packet's block ids itself sets `static constexpr bool rewrites_block_ids = true`,
  so `handle` skips `rewriteBlockIds` for it.

## Block states

Every version renumbers some block states, so every hop carries the block types whose states
change across it. A block's network id is BDS's `Block::computeRawSerializationIdHashForNetwork`:
FNV-1a over its name and states written as little-endian NBT. `protocol/block.h` computes it the
same way, so a hop is stated as readable rules and the ids follow from them.

- **The rules are generated C++, committed.** `protocols/v<from>_to_v<to>/block_states.h` lists one
  `BlockStateUpdate` per block type that changes: every value of each of its states, then the name
  it takes (`rename`), the states it gains (`add`) and the ones it loses (`remove`), each in SNBT,
  which `protocol/tag_parser.h` reads. The records are `static constexpr` data with no code to build
  them, which keeps the translation unit quick to compile. It is `// clang-format off`, and it is
  never edited by hand. `getNewBlockStateId` hashes every state of those blocks once per hop and
  looks ids up hop by hop; a block no hop changes keeps its id, which is what an add-on block needs.
- **`neighbor_blocks.h` is generated for a hop that gives blocks states**: a `NeighborBlock` for
  every block the older version can send, as the newer version has it, holding what BDS reads off a
  block when it works out a neighbour's connections: its `BlockConnectionRuleComponent` and connection
  id, flame odds, redstone components and virtuals, and the faces it gives full support on. A record
  is one block type's states that share all of these, so a type where one depends on a state, such
  as a stair's facing, gets a record for each.
- **`data/` holds each version's block data, committed**, laid out as ViaVersion's Mappings repo
  lays out its own. `data/v<protocol>.json` is the fields endweave reads from one version's
  bedrock-dumper `block_types.json` and `block_palette.json`, one entry per line, sorted, each state
  a JSON value (bool for a `ByteTag`, number for an `IntTag`, string for a `StringTag`) and each
  field left out at its default, then the item names from its `items.json`. `uv run tools/import_bedrock_data.py <bedrock-data-<version>> <protocol>`
  writes it. `data/fallbacks/v<from>_to_v<to>.json` is written by hand: `block_types` names the
  fallback for each block type the `to` version lacks. `data/` stays out of the sdist and the wheel.
- **`uv run tools/generate_block_states.py` writes the rules from `data/`.** The script mirrors
  BDS's classes and methods (`Block`, `CompoundTag`, `NbtIo`, `StringByteOutput`) and checks its hash
  against every id in the data. Going up, a block gains the states the newer version added, at that
  version's default, and loses the ones it removed. Going down, it loses what the older version
  lacks, and a block type the older version lacks is renamed to its fallback, which should keep the
  block's shape so the client's movement prediction agrees with the server. A state whose values
  change stops the generator rather than being guessed.
- **`rewriter/block.h` is `BlockRewriter`.** `rewriteBlockIds` finds every block id by its type,
  `bp::BlockRuntimeId`, which bedrock-protocol gives every member BDS types `BlockRuntimeId`, and
  `should_translate_v` covers every packet that carries one. `SerializedNetworkItemInstanceDescriptor`
  is the one member BDS types `int32_t`, and the walker finds it by name.
- **Ids the schema cannot type are `Rewriter`s** in `rewriter/block.h`: the particle `LevelEvent`s,
  the block sound events, and the sub-chunks in `LevelChunkPacket` and `SubChunkPacket`. Actor data
  keys 16 and 23, and `Variant` on a falling block, go through `BlockRewriter::rewriteActorData` from
  the actor rewriters in `protocols/rewriters.h`.
- **`protocol/sub_chunk.h` reads and writes network sub-chunks**, BDS's `SubChunk` and
  `SubChunkStorage<Block>`. A storage keeps its packed indices as they were read until a block in it
  is set, so a hop that only renumbers rewrites the palette and copies the rest. Biomes, border
  blocks and block actors after the sub-chunks are copied as they are.
- **States BDS works out from neighbouring blocks are `ConnectionData<From, To>`**
  (`protocol/block_connection.h`), specialised in the hop that adds them, as ViaVersion's
  `ConnectionData` dispatches to its `ConnectionHandler`s. `BlockRewriter::rewriteSubChunks` takes a
  packet's sub-chunks a hop at a time: it renumbers the palettes, then gives every block the hop
  connects the id its neighbours make it, reading the sub-chunks in the same packet, then the blocks
  the client was sent before. A neighbour in neither counts as air. Each handler is BDS's rule as
  written: it reads its neighbours through
  `findNeighborBlock`, and `ConnectedBlocks` holds the id its own blocks take for every way of setting
  the states. `protocols/v2168_to_v2193/block_connections.h` has `StairConnectionHandler` (BDS's
  `CornerConnectionCallback`, `minecraft:corner`) and `StandardConnectionHandler` (BDS's
  `StandardConnectionCallback`, `minecraft:connection_*` on fences, panes, bars and tripwire).
  `protocols/v2193_to_v2225/block_connections.h` has `ChorusPlantConnectionHandler` and
  `FireConnectionHandler` (BDS's two `FunctionConnectionCallback`s) and `RedstoneConnectionHandler`
  (BDS's `RedStoneWireBlock`, `redstone_*`).
- **`BlockConnectionStorage` keeps the blocks a client was sent**, by sub-chunk, as the client has
  them, for a translator whose chain adds connection states; other translators keep nothing. Each
  pipeline `Translator` owns one, and `handle` passes it to the rewriters that take it. Chunk packets
  fill it, `UpdateBlock`, `UpdateBlockSynced` and `UpdateSubChunkBlocks` give a block in layer 0 the
  states its kept neighbours make it and record the result, `NetworkChunkPublisherUpdate` drops the
  columns the client lets go of, and `ChangeDimension` drops everything.
- **A block sent before its sub-chunk is sent again after it.** BDS sends the block changes of a
  chunk that just loaded around its `LevelChunkPacket`, before the client has asked for the
  sub-chunks, so such a block is connected with no neighbours. The client lays it over the sub-chunk that comes
  later, so the storage notes it, and storing that sub-chunk queues it to be sent again as unsent.
- **A sub-chunk reshapes the kept blocks along its border.** One connected before its neighbour
  sub-chunk arrived read that neighbour as air, so `BlockRewriter::updateBorders` works out again the
  border blocks of the kept sub-chunks beside each stored one, skipping any whose palette holds no
  block a hop connects, and queues those that change.
- **A changed block reconnects the kept blocks around it.** `BlockRewriter::updateNeighbors` works
  out again every kept block in the 3×3×3 around a block those three packets change, since redstone
  reads the wire a block up or down on each side, past ViaVersion's six faces. The server sends
  nothing for a block whose states change that way, so the storage keeps it as unsent, and
  `Translator.take_scheduled_packets()` hands over an `UpdateBlock` for each. The plugin's
  `_schedule_send` sends them every tick and lets their send event through untranslated: ViaVersion writes such packets
  below its encoder, but one sent from inside Endstone's send event goes out ahead of the packet in
  hand, and before Endstone 0.11.14 it overwrites the buffer a broadcast reuses for the next player.
- **The client cache is turned off.** The server's blob ids hash the sub-chunks it serializes, so
  they stop naming what the client is sent once a palette is rewritten. `ClientCacheStatus` is
  rewritten to off for every pair.

## Items

The server's `ItemRegistryPacket` gives every item its numeric id, so ids need no mapping; a client
older than the server is only offered items its version lacks.

- **`uv run tools/generate_items.py` writes `protocols/v<from>_to_v<to>/item_mappings.h`** from the
  item names in `data/`, for a hop whose `from` has items its `to` lacks: an `ItemMappings`
  specialisation listing them, which `getNewItemName` (`protocol/item.h`) reads hop by hop, as
  ViaVersion's `MappingData#getNewItemId` does, returning none for a missing item.
- **The registry keeps every item**, so a stack of one the client lacks still resolves, and the
  `ItemRegistryPacket` rewriter notes their ids in the translator's `ItemRegistryStorage`. The
  `CreativeContent` rewriter leaves those items out, and any group they leave empty, and the
  `CraftingData` rewriter the recipes and mixes that take or make one (`rewriter/item.h`). MOT
  does the same; ViaBackwards instead shows a fallback item, which needs the real one restored on
  the way back.

## Transformers

`Transformer` is the analogue of ViaVersion's `ValueTransformer`, in the shape of `std::formatter`:
a trait declared undefined in `protocol/transform.h` and specialized per source/destination pair
that changes shape. That header also holds the `ew::transform` / `ew::transform_to` /
`ew::transform_into` / `ew::transform_members` call surface and the `std::optional` / `std::vector` /
`std::map` / `std::variant` specializations.

- **The context is the first parameter, and the destination lives in it.**
  `Transformer<From, To>::transform(Context<To> &ctx, From &&from)` returns `void` and fills
  `ctx.out()`. `ctx.with(child)` makes the context a nested field writes through, so the cancel
  reaches every level. `Context<To>` is a concrete type inside a specialization, which is what keeps
  the body out of line in the `.cpp`.
- **A packet the destination cannot express is dropped through the context.** `ctx.cancel()` sets
  the flag and the transform returns immediately; whatever it wrote is never read. `chain` stops at
  the hop that cancelled, `handle` skips the encode, and `translate` returns `None`. A refusal is a
  deliberate drop, not an error. The live cases are an inventory action or container type the other
  version has no name for, and a `ServerboundPackSettingChangePacket` whose 2193 value is a list of
  strings, which 2168 has no arm for.
  @see ViaVersion `PacketWrapper#cancel`.
- **Most pairs write themselves.** Where both types have the same `struct_name` and every member of
  the destination is filled by a same-named member of the source, `memberwise_complete_v` holds and
  the generic `Transformer` copies member by member, transforming the members whose types differ.
  Matching members alone are not enough: `NormalTransactionData` and `InventoryMismatchData` look
  alike. A hand-written body can call `ew::transform_members` for the bulk and then write only what
  differs.
- **Never hand-write a pair the generic one covers.** Before declaring a specialization,
  static-assert `!memberwise_complete_v<From, To>`; if it holds, the body is a copy the compiler
  already writes. After a schema change, re-probe every existing pair and delete the ones that became
  complete, along with any module left empty (its header, `.cpp`, `CMakeLists.txt` line and
  `transform.h` include). A variant-carrying pair qualifies too, provided the alternatives line up by
  index.
- **A variant keeps its index.** It is the discriminant BDS writes, so alternatives are matched by
  index, never by shape.
- **One directory per hop and direction,** `protocols/v<from>_to_v<to>/`, one module per subsystem
  (`inventory`, `chunk`, `sound`, ...). Each module's header declares the specializations and its
  `.cpp` defines them; the `.cpp` is listed in that directory's `CMakeLists.txt`, and the header in
  its `transform.h`, which `handler.h` includes. The bodies compile in parallel, and editing one
  recompiles one file rather than every table.
- **`Transformable<From, To>` is the availability test,** and it asks whether a call to
  `Transformer<From, To>::transform` is well-formed with a `Context<To>` and returns `void`, not
  whether the specialization exists.
- **A transform consumes its source.** It takes an rvalue reference and moves every field that owns
  storage. A `const` source is a compile error rather than a copy.
- **In a hand-written body, assign every field explicitly, in declaration order,** through
  `auto &to = ctx.out();`. Only a field whose shape actually changed carries logic.
- **Call through `ew::transform`,** which returns a proxy that takes its destination from the
  assignment target: `to.slots = ew::transform(ctx, std::move(from.slots));`. The `std::move` is not
  optional. Where there is no destination to deduce from, use `ew::transform_to<Dest>(ctx,
  std::move(x))`; where the destination exists, `ew::transform_into(ctx, std::move(x), dest)`.
  Qualifying is not optional either: inside a `Transformer<...>::transform` body the unqualified
  name finds the member.
- **A renumbered enum is mapped by name, never by a shift.** New enumerators land before a trailing
  sentinel, so numbers move every version. Map with
  `bp::enum_cast<To>(bp::enum_name(from)).value_or(<To's sentinel>)`, or cancel where no fallback is
  honest. Do not add a shift table.
- **An enum that changes every version is mapped once, for every pair,** as its own partial
  specialization in `protocols/enums.h`, keyed on `bp::enum_type_name` and carrying its own fallback.
  Add a case there only for an enum that keeps moving; there is no default, so any other enum still
  fails the build until its hop writes a `Transformer`.
- **Establish what changed by static-asserting `is_same_v` on every field pair,** not by reading the
  two structs side by side. A versioned type can be nested where the packet's own name gives no sign.
- **A field can migrate into a nested type rather than be renamed.** A name that vanishes from the
  top level is worth hunting for one level down before treating it as dropped.
- **A field with no source is invented, and says so** in an `// ENDWEAVE:` note. Each of these is a
  candidate for `ctx.cancel()` instead; none should be quietly widened into looking faithful.

## Correspondence map

| endweave | ViaVersion (api / common), unless noted |
| --- | --- |
| `plugin.py` `EndweavePlugin` | platform plugin, `ViaDecodeHandler` / `ViaEncodeHandler` |
| `connection.py` `Connection` | `UserConnection` + `UserConnectionImpl`, with `ProtocolInfo` folded in |
| `connection.py` `ConnectionManager` | `ConnectionManager` + `ConnectionManagerImpl` |
| `protocol/base.py` `BaseProtocol` | `InitialBaseProtocol`, `ServerboundBaseProtocol1_7`, `ClientboundBaseProtocol1_7` |
| `protocol/version.py` `ProtocolVersion` | `ProtocolVersion` |
| `config.py`, `commands.py`, `debug.py`, `update.py` | `Config` / `ConfigurationProvider`, `ViaCommandHandler`, `DebugHandler`, `UpdateUtil` |
| `protocol/context.h` `Context` | `PacketWrapper` |
| `protocol/transform.h` `Transformer` | `ValueTransformer` |
| `protocol/rewrite.h` `Rewriter` | none by name; the job is ViaVersion's per-packet `PacketHandler` lambda |
| `protocol/block.h` `getNewBlockStateId` | `MappingData#getNewBlockStateId` |
| `rewriter/block.h` `BlockRewriter` | `BlockRewriter` |
| `protocol/block_connection.h` `BlockConnectionStorage` | `BlockConnectionStorage`, read as `PacketBlockConnectionProvider` reads it |
| `protocol/sub_chunk.h` `SubChunk`, `SubChunkStorage` | `ChunkSection`, `DataPalette` |
| `protocol/block_connection.h` `ConnectionData` | `ConnectionData` |
| `rewriter/block.h` `BlockRewriter::updateNeighbors` | `ConnectionData#update` |
| `plugin.py` `_schedule_send` | `PacketWrapper#scheduleSend` |
| `protocols/v2168_to_v2193/block_connections.h` `StairConnectionHandler` | `StairConnectionHandler` |
| `protocols/v2168_to_v2193/block_connections.h` `StandardConnectionHandler` | `AbstractFenceConnectionHandler` and its subclasses |
| `protocols/v2193_to_v2225/block_connections.h` `ChorusPlantConnectionHandler`, `FireConnectionHandler`, `RedstoneConnectionHandler` | the handlers of the same names |
| `data/v<protocol>.json`, `data/fallbacks/v<from>_to_v<to>.json` | Mappings repo `mappings/mapping-<version>.json`, `mappings/diff/` |
| `tools/import_bedrock_data.py`, `tools/generate_block_states.py`, `tools/generate_items.py` | Mappings repo `MappingsGenerator`, `MappingsOptimizer` |
| `protocol/item.h` `ItemMappings`, `getNewItemName` | ViaBackwards `ItemMappings`, `MappingData#getNewItemId` |
| `rewriter/item.h` `ItemRewriter` | `ItemRewriter` |
| `protocol/handler.h` `PacketHandler` | `PacketHandler` (`api/protocol/remapper/`) |
| `protocol/handler.h` `Translator` | `Protocol` by role, Velocity `ProtocolRegistry` by behaviour |
| `protocol/handler.h` `getTranslator` | Velocity `StateRegistry.PacketRegistry#getProtocolRegistry` |
| `protocol/version.h` `ProtocolVersions` | Velocity `ProtocolVersion#SUPPORTED_VERSIONS` / `#getProtocolVersion(int)` |

## Sources of truth

1. **bedrock-protocol**, for anything the codec reads or writes. A wrong wire format is fixed there,
   not worked around here.
2. **EndstoneMC/protocol-docs**, one branch per BDS release, for what changed between versions.
3. **ViaVersion / ViaBackwards**, for translation semantics and names.
4. **gophertunnel**, CloudburstMC/Protocol and pmmp, for cross-checking field names only. They are
   reverse-engineered and have been wrong about wire formats before.

## Building and testing

Requires Python 3.11+, [uv](https://docs.astral.sh/uv/) and a C++23 compiler. scikit-build-core
drives CMake (3.28+) and nanobind; bedrock-protocol is fetched as the release pinned by
`BEDROCK_PROTOCOL_VERSION` in `CMakeLists.txt`. The standard library only has to agree with itself,
so the compiler's default is used and linked statically.

```shell
uv sync --extra dev                                    # build the engine and install everything
uv run pytest                                          # tests
uv run ruff check src tests && uv run ruff format --check src tests
uv run mypy src/endweave/ --strict                     # CI type-checks src only
uvx cibuildwheel --platform linux                      # the manylinux wheels CI tests and releases publish
```

- **Rebuild the engine after editing C++** with
  `env -u VIRTUAL_ENV uv sync --extra dev --reinstall-package endstone-endweave`. The distribution
  is `endstone-endweave`; `--reinstall-package endweave` matches nothing, exits 0, and leaves the
  old `_pipeline.abi3.so` in place. Check its mtime (`endweave._pipeline.__file__`) before trusting a
  result. A bare `cmake --build` of `build/<wheel tag>` fails: that tree belongs to
  scikit-build-core's isolated environment.
- **Building against a local bedrock-protocol checkout** is the `BEDROCK_PROTOCOL_SOURCE_DIR` cache
  variable, passed through scikit-build-core as
  `-C cmake.define.BEDROCK_PROTOCOL_SOURCE_DIR=<absolute path>`.
- **`_pipeline.pyi` is generated** by `nanobind_add_stub` from the built module and committed.
  stubgen leaves `TranslationError` out, so `_pipeline.pat` supplies it. Keep the stub in step with
  the bindings.
- **A green build is the translation gate** (see **The handler layer**); the tests cover the plugin,
  the base protocol, the version registry and the engine's Python surface. `tests/` mirrors
  `src/endweave/`: add a case to the file that owns the subject, or start one.
- **Releases** run the `Release` workflow by hand with a version. It stamps `## [Unreleased]` in
  `CHANGELOG.md`, tags, and publishes the sdist and wheels to PyPI. It calls `build.yml`, which lints and builds them with
  cibuildwheel (a cp311 and a cp312 abi3 wheel, manylinux_2_28 and win_amd64) and tests each
  wheel on the runner, since endstone does not install in the manylinux container. A dry run builds
  and tests them without publishing.

## Code Style

### C++

- C++23, clang-format (see `.clang-format`). Classes/enums `CamelCase`, methods `camelBack`,
  private members `lower_case_` (trailing underscore), locals/params `lower_case`, compile-time
  predicates as `snake_case_v` variable templates.
- **A move is fine, a copy is not.** Packets are moved through the chain, never copied, and the
  budget is one move for the whole walk however many versions it crosses: the base case's
  `ctx.out() = std::move(from)`.
- **Simple over clever, and nothing speculative.** No metaprogramming that today's version set does
  not exercise, and no helper that exists only to name two lines.
- **Comments: write almost none.** A doc comment is one short line of what the thing is, and only
  where the name does not say it. Beyond that, two kinds: a single `@see ViaVersion Foo#bar.` on a
  type or method with an upstream counterpart, and an `// ENDWEAVE:` note where a conversion is
  lossy, invented or refused. No rationale, no `@param` or `@return` blocks.
- Per-module files include the one bedrock-protocol module they need, `<bedrock/protocol/<module>.h>`.
  `protocol/handler.h` is the exception and must take the `<bedrock/protocol.hpp>` umbrella:
  `bp::packet_of_t` is `void` for an id whose module is not included, and two `void`s compare equal,
  so a lone module include makes unmodelled packets read as unchanged.
- `namespace bp = bedrock::protocol;`, declared after the includes and above `namespace endweave`.
  Versioned types are spelled through it, `bp::StartGamePacket_<2193>`. `namespace ew = endweave;`
  follows the same placement, but only in a `.cpp`, never a header.

### Python

- ruff with a 120-character line (`I`, `E`, `F`, `W`), and mypy `--strict` over `src/endweave/`.
- A module docstring says what the module ports and what it leaves out, with a `See Also:` block of
  the upstream classes. Function docstrings are Google style (`Args:`, `Returns:`, `Raises:`);
  properties and methods whose name says it all get none.

### Prose

- CHANGELOG.md strictly follows [Keep a Changelog 1.1.0](https://keepachangelog.com/en/1.1.0/):
  - Write for server admins, not machines.
  - Every version gets an entry, latest first, with its release date and a reference-style link at
    the bottom.
  - Group changes by type: Added, Changed, Deprecated, Removed, Fixed, Security.
  - Include only what a server admin sees, such as which clients can join, config keys and fixes.
    Omit internal implementation details: refactors, engine plumbing, build and tooling changes.
  - One short sentence per entry: state what changed, not why or how. The why and how belong in the
    commit message.
  - Append the issue number when a fix closes an issue, e.g. `(#55)`.
  - Do not list fixes for bugs introduced by unreleased work.
  - Do not add caveats that read as restrictions.
  - Prefix breaking changes with `**BREAKING**:` in the Changed or Removed section.
  - Plain declarative sentences. No em-dash pivots, mirrored clauses or other rhetorical flourishes.
- No double dashes (`--`) in user-facing prose.
- No `Co-Authored-By` trailer on commits.
