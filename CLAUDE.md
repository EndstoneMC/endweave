# Endweave

ViaVersion-like protocol translation plugin for Minecraft Bedrock Edition (Endstone).
Supports chained protocol translation between arbitrary Bedrock version pairs.
Currently translates between protocol 924 (MC 1.26.0) and 944 (MC 1.26.10).

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

1. Add `v1_27_0 = ProtocolVersion(960, "1.27.0", "r27_u0")` to `protocol/versions.py`
2. Run tools to fetch + parse + diff the new protocol docs
3. Create `protocol/v944_to_v960/` with:
   - `protocol.py` -- `SERVER_PROTOCOL = 944`, `CLIENT_PROTOCOL = 960`, `create_protocol()`
   - `handlers/` -- version-specific packet rewriters
4. Register in `plugin.py`: `self._register_protocol(create_v944_to_v960())`

The chaining system automatically handles multi-step gaps (e.g. v960 client -> v924 server).

## Reference

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
