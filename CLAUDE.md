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
client's; `Login` (1) gets the same rewrite, since BDS checks it again. Translators come from a
`functools.cache` over `_pipeline.Translator`, so every connection between the same two versions
shares one. Ends the engine reads as one version, or does not carry, get no translators and no
rewrite. The blocked version gate runs on `PlayerLoginEvent`.

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
- `protocol/version.h` `ProtocolVersions::SUPPORTED_VERSIONS` is what the engine translates: 2168
  and 2192, a sorted line that `step(from, to)` routes along one hop at a time, so there is no path
  search. `WIRE_IDENTICAL` routes 2169 (26.45) as 2168, and `handler.h` static-asserts that
  claim against the schema, so a version that stops being identical fails the build.

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
   per-version `PacketType` enums and name-based auto-mapping (Bedrock ids are stable), `MappingData`,
   the rewriter hierarchy, `Type<T>`, and the platform-split config options. Of `PacketWrapper`,
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
  `SetPlayerFurnaceOptions` (351) is modelled at 2192 alone, so 2192 towards 2168 cancels it.
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
  `From == To`, and `BaseProtocol` installs no translators when both ends resolve to one version.
  The guard is load-bearing because a `Rewriter` can force translation on its own.
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
  translation reproduces it, and the heartbeat `LevelSoundEvent` inside actor data. A rewriter for
  one pair goes in that pair's directory.

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
  version has no name for, and a `ServerboundPackSettingChangePacket` whose 2192 value is a list of
  strings, which 2168 has no arm for.
  @see ViaVersion `PacketWrapper#cancel`.
- **Most pairs write themselves.** Where both types have the same `struct_name` and every member of
  the destination is filled by a same-named member of the source, `memberwise_complete_v` holds and
  the generic `Transformer` copies member by member, transforming the members whose types differ.
  Matching members alone are not enough: `NormalTransactionData` and `InventoryMismatchData` look
  alike. A hand-written body can call `ew::transform_members` for the bulk and then write only what
  differs.
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
- **Assign every field explicitly, in declaration order,** through `auto &to = ctx.out();`. Only a
  field whose shape actually changed carries logic.
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

Requires Python 3.10+, [uv](https://docs.astral.sh/uv/) and a C++23 compiler. scikit-build-core
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
  cibuildwheel (cp310, cp311 and a cp312 abi3 wheel, manylinux_2_28 and win_amd64) and tests each
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
  Versioned types are spelled through it, `bp::StartGamePacket_<2192>`. `namespace ew = endweave;`
  follows the same placement, but only in a `.cpp`, never a header.

### Python

- ruff with a 120-character line (`I`, `E`, `F`, `W`), and mypy `--strict` over `src/endweave/`.
- A module docstring says what the module ports and what it leaves out, with a `See Also:` block of
  the upstream classes. Function docstrings are Google style (`Args:`, `Returns:`, `Raises:`);
  properties and methods whose name says it all get none.

### Prose

- CHANGELOG.md follows Keep a Changelog. Entries state what was broken and what is fixed, for server
  admins, not why or how.
- No double dashes (`--`) in user-facing prose.
- No `Co-Authored-By` trailer on commits.
