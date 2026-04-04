# Endweave

ViaVersion-like protocol translation plugin for Minecraft Bedrock Edition (Endstone).
Supports chained protocol translation between arbitrary Bedrock version pairs.
Currently translates between protocols 859 (MC 1.21.120) through 944 (MC 1.26.10).

## Architecture

- **Plugin** (`plugin.py`): Endstone plugin entry point, registers event handlers and protocols
- **Pipeline** (`pipeline.py`): `ProtocolPipeline` -- pure dispatcher routing packets through base + version-specific protocols
- **Connection** (`connection.py`): `UserConnection` (per-player state), `ConnectionState`, and `ConnectionManager`
- **Debug** (`debug.py`): `DebugHandler` with per-packet-ID filtering and structured log format
- **Exception** (`exception.py`): `InformativeException` with fluent `.set()` context for error reporting
- **Codec** (`codec/`): Binary reader/writer for Bedrock packet serialization (varint, zigzag, LE/BE integers)
- **Protocol** (`protocol/`): Translation infrastructure + version-specific protocol modules
  - `__init__.py`: `Protocol` class and `PacketHandler` type alias
  - `direction.py`: `Direction` enum (CLIENTBOUND / SERVERBOUND)
  - `versions.py`: Central `ProtocolVersion` registry (single source of truth for all known versions)
  - `packet_ids.py`: Shared `PacketId` enum (Bedrock IDs are stable across versions)
  - `manager.py`: `ProtocolManager` with BFS chaining for multi-step version gaps + base protocol support
  - `base.py`: `create_base_protocol()` -- always-on handlers for version detection and disconnect logging
  - `rewriter.py`: Shared field conversion helpers (BlockPos, ActorData, StructureSettings)
  - `v924_to_v944/`: Version-specific protocol module (exports `create_protocol`, `SERVER_PROTOCOL`, `CLIENT_PROTOCOL`)
- **Tools** (`tools/`): Offline scripts to fetch protocol docs, parse DOT + JSON files, generate diffs

## API Constraints (Endstone 0.11.2)

- `PacketReceiveEvent` / `PacketSendEvent`:
    - `packet_id`: **readonly** int
    - `payload`: **read/write** bytes (excludes header)
    - `cancel()`: drops the packet
- `packet_id` is readonly -- we cannot remap packet IDs (not needed: Bedrock IDs are stable across versions)
- Endstone fires `PacketReceiveEvent` for pre-login packets including `RequestNetworkSettings`
- `ServerListPingEvent.minecraft_version_network` is read/write

## Development

- `uv run tools/diff.py [924 944]` -- diff two version JSONs (auto-fetches and parses if needed)
- `uv run tools/fetch.py [924 944...]` -- fetch protocol DOT + JSON files from BedrockProtocol repo
- `uv run tools/parse.py [924 944...]` -- parse DOT + JSON -> structured JSON
- `uv run pytest tests/` -- run unit tests
- `uv run ruff check src/ tests/` -- lint
- `uv run mypy src/endstone_endweave/ --strict` -- type check

## Key Design Decisions

- Fast-path skip: serverbound uses `not connection.needs_translation`, clientbound uses `not connection.active`
- Only deserialize packets that have registered rewriters (pass-through by default)
- Base protocol detects client version from `RequestNetworkSettings` and logs disconnects (always-on, separate from version-specific protocols)
- Version-specific protocols rewrite `RequestNetworkSettings` protocol field to server's version so BDS accepts newer clients
- Cancel new serverbound packet IDs that the older server doesn't understand
- Protocol chaining: each module handles one adjacent version step; pipeline chains them for larger gaps
- Login handlers use `connection.server_protocol` (not hardcoded constants) for version-agnostic rewriting

## Adding a New Version Pair

### 1. Register the version

Add to `protocol/versions.py`:

```python
v1_27_0 = ProtocolVersion(960, "1.27.0", frozenset({"1.27.0"}))
```

Include all MC sub-versions sharing this protocol in `included_versions`. Add to the `VERSIONS` dict.

### 2. Diff the protocols

```
uv run tools/fetch.py 944 960
uv run tools/parse.py 944 960
uv run tools/diff.py 944 960
```

The diff output drives every handler decision below.

### 3. Create both direction modules

Every version pair needs **two** directories (one per direction):

- `protocol/v944_to_v960/` (v944 server <- v960 client)
- `protocol/v960_to_v944/` (v960 server <- v944 client)

Each directory contains:

```
__init__.py          re-exports create_protocol
protocol.py          SERVER_PROTOCOL, CLIENT_PROTOCOL, create_protocol()
handlers/
    __init__.py      empty
    start_game.py    always needed
    ...              one file per change category
```

If versions are wire-identical (like v859/v860), `protocol.py` returns a bare `Protocol()` with no handlers.

### 4. Write the protocol.py factory

```python
SERVER_PROTOCOL = 944
CLIENT_PROTOCOL = 960

def create_protocol() -> Protocol:
    p = Protocol(server_protocol=SERVER_PROTOCOL, client_protocol=CLIENT_PROTOCOL)
    # 1. Cancel unknown packets
    # 2. Register clientbound/serverbound handlers
    # 3. Sound remapping
    return p
```

Registration methods:

- `p.register_clientbound(PacketId.X, handler)` server->client rewriter
- `p.register_serverbound(PacketId.X, handler)` client->server rewriter
- `p.cancel_serverbound(PacketId.X, ...)` drop packets the older server does not know
- `p.cancel_clientbound(PacketId.X, ...)` drop packets the older client does not know

**Direction semantics:** In `v944_to_v960` (v944 server <- v960 client), clientbound rewrites v944 output for v960 client (add new fields), serverbound rewrites v960 input for v944 server (strip new fields, cancel new packet IDs).

### 5. Write handlers

Handler signature: `def rewrite_X(wrapper: PacketWrapper) -> None`

Wrapper API:

| Operation | Effect | Use case |
|---|---|---|
| `passthrough(TYPE)` | read + write | Unchanged field |
| `read(TYPE)` | read only | Remove a field |
| `write(TYPE, val)` | write only | Add a field |
| `map(OLD, NEW)` | read OLD, write NEW | Encoding change |
| `passthrough_all()` | copy remaining bytes | Tail of unchanged packet |

Common patterns by change type:

- **Field added**: passthrough up to insertion point, `write(TYPE, default)`, continue
- **Field removed**: passthrough up to field, `read(TYPE)` to discard, continue
- **Type changed**: `map(OLD_TYPE, NEW_TYPE)` (e.g. `NETWORK_BLOCK_POS` -> `BLOCK_POS`)
- **Enum shift**: `read(VAR_INT)` + offset + `write(VAR_INT, shifted)`
- **Struct changed**: `map(LEVEL_SETTINGS_V944, LEVEL_SETTINGS_V960)` using versioned compound types
- **Field reordered**: read fields into locals, passthrough middle, write fields in new order

### 6. Sound remapping

Almost every version pair shifts `LevelSoundEvent` IDs. Add `UNDEFINED_V960` to `LevelSoundEvent` in `codec/types/enums.py`, then in `protocol.py`:

```python
def _remap_sound(v: int) -> int:
    if v >= LevelSoundEvent.UNDEFINED_V944:
        return v + (LevelSoundEvent.UNDEFINED_V960 - LevelSoundEvent.UNDEFINED_V944)
    return v

sound = SoundRewriter(
    sound_remap=_remap_sound,
    actor_data_int_remappers={ActorDataIDs.HEARTBEAT_SOUND_EVENT: _remap_sound},
)
sound.register(p)
```

For the reverse direction, the remap collapses new IDs to the older version's `UNDEFINED`. `SoundRewriter.register()` covers `LEVEL_SOUND_EVENT`, `SET_ACTOR_DATA`, `ADD_ACTOR`, `ADD_ITEM_ACTOR`, `ADD_PLAYER`.

### 7. Register in plugin.py

Add both factory imports to `plugin.py` and append to the `for factory in (...)` loop in `on_enable`.

### 8. Compound types (if needed)

When a compound struct's wire format changes (LevelSettings, StructureSettings, Commands):

1. Create a versioned type constant in `codec/types/<module>.py` (e.g. `LEVEL_SETTINGS_V960`)
2. Export through `codec/types/__init__.py` and `codec/__init__.py`
3. Use `wrapper.map(OLD_VERSION, NEW_VERSION)` in handlers

### Gotchas

- **StartGame always needs a handler**: contains LevelSettings (version-dependent) and block registry checksum (must zero with `read(INT64_LE)` + `write(INT64_LE, 0)` to skip client validation)
- **Both directions required**: the BFS chaining graph needs edges in both directions or that version pair is unreachable
- **Sound remap in both directions**: forward shifts up, reverse collapses new IDs
- **Inline comments**: must use exact field names from `data/v{N}_packets.json`

The chaining system automatically handles multi-step gaps (e.g. v960 client -> v924 server).

## Reference

- **BDS headers are the single source of truth** for packet formats, enum values, and struct layouts; if the headers path is not known, ask the user for it
- Never use CloudburstMC as a reference; it contains many incorrect enum values, wrong field orders, and fabricated constants (e.g. AnimatePacket RowLeft/RowRight) that do not exist in BDS
- All protocol constants must be `IntEnum` classes in `codec/types/enums.py` matching the BDS C++ `enum class` names and values
- Use `enum_to_label()` / `label_to_enum()` helpers for v898's string-serialized enums (a v898-specific quirk, not a general pattern)
- Always refer to `vendor/ViaVersion/` code as a reference when working on protocol translation logic
- If the `vendor/ViaVersion/` folder does not exist, clone it from https://github.com/ViaVersion/ViaVersion
- Use `See Also:` docstring sections with fully qualified Java paths when referencing ViaVersion classes

## Git

- Never add a Co-Authored-By line for Claude in commit messages
- Maintain a `CHANGELOG.md` following https://keepachangelog.com/en/1.0.0/
- CHANGELOG audience is server admins and non-technical users; write plain-language entries describing user-visible behavior, not internal implementation details (no class names, no API names, no refactoring notes)
- No double dashes in user-facing prose (README, CHANGELOG)

## Code Style

- Never use non-ASCII characters in code (no em dashes, smart quotes, fancy arrows, etc.) -- use only plain ASCII
- Never use `from __future__ import annotations`
- Never use `__slots__`
- Use Google-style docstrings (`Args:`, `Returns:`, `Attributes:` sections) for non-trivial functions and classes
- Trivial one-liner docstrings are fine for self-explanatory functions (e.g. `read_byte`, `write_int_le`)
- Inline comments next to wrapper.passthrough/read/write calls must use the exact field names from `data/v924_packets.json` (or v944 for new fields)
- Add `__all__` (alphabetically sorted) to `__init__.py` files that re-export symbols; skip empty package markers
- Use `_T` (underscore prefix) for TypeVar names
- Prefer `@property` over getter methods for no-arg methods that return state

## Tooling

- Always use `uv pip` instead of `pip` for package management
- Use `uv run` to execute scripts (e.g. `uv run tools/diff.py`)
