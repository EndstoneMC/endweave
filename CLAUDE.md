# Endweave

ViaVersion-like protocol translation plugin for Minecraft Bedrock Edition (Endstone).
Supports chained protocol translation between arbitrary Bedrock version pairs.
Currently translates between protocol 924 (MC 1.26.0) and 944 (MC 1.26.10).

## Architecture

- **Plugin** (`plugin.py`): Endstone plugin entry point, registers event handlers and protocols
- **Pipeline** (`pipeline.py`): `ProtocolPipeline` -- pure dispatcher routing packets through base + version-specific protocols
- **Connection** (`connection.py`): `UserConnection` (per-player state) and `ConnectionManager`
- **Codec** (`codec/`): Binary reader/writer for Bedrock packet serialization (varint, zigzag, LE/BE integers)
- **Protocol** (`protocol/`): Translation infrastructure + version-specific protocol modules
  - `versions.py`: Central `ProtocolVersion` registry (single source of truth for all known versions)
  - `packet_ids.py`: Shared `PacketId` enum (Bedrock IDs are stable across versions)
  - `manager.py`: `ProtocolManager` with BFS chaining for multi-step version gaps + base protocol support
  - `base.py`: `Protocol` class and `PacketAction` result type
  - `base_protocol.py`: `BaseProtocol` -- always-on handlers for version detection and disconnect logging
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

- `python tools/fetch_protocol_docs.py [tags...]` -- fetch protocol DOT + JSON files from BedrockProtocol repo
- `python tools/parse_protocol_docs.py [tags...]` -- parse DOT + JSON -> structured JSON
- `python tools/generate_diff.py [old new]` -- diff two version JSONs
- `pytest tests/` -- run unit tests

## Key Design Decisions

- Fast-path skip when `not connection.needs_translation` (no deserialization overhead for matching clients)
- Only deserialize packets that have registered rewriters (pass-through by default)
- Base protocol detects client version from `RequestNetworkSettings` and logs disconnects (always-on, separate from version-specific protocols)
- Version-specific protocols rewrite `RequestNetworkSettings` protocol field to server's version so BDS accepts newer clients
- Cancel new serverbound packet IDs that the older server doesn't understand
- Protocol chaining: each module handles one adjacent version step; pipeline chains them for larger gaps
- Login handlers use `connection.server_protocol` (not hardcoded constants) for version-agnostic rewriting

## Adding a New Version Pair

1. Add `R27_U0 = ProtocolVersion(960, "1.27.0", "r27_u0")` to `protocol/versions.py`
2. Run tools to fetch + parse + diff the new protocol docs
3. Create `protocol/v944_to_v960/` with:
   - `protocol.py` -- `SERVER_PROTOCOL = 944`, `CLIENT_PROTOCOL = 960`, `create_protocol()`
   - `handlers/` -- version-specific packet rewriters
4. Register in `plugin.py`: `self._register_protocol(create_v944_to_v960())`

The chaining system automatically handles multi-step gaps (e.g. v960 client -> v924 server).

## Reference

- Always refer to `vendor/ViaVersion/` code as a reference when working on protocol translation logic
- If the `vendor/ViaVersion/` folder does not exist, clone it from https://github.com/ViaVersion/ViaVersion

## Git

- Never add a Co-Authored-By line for Claude in commit messages

## Code Style

- Never use non-ASCII characters in code (no em dashes, smart quotes, fancy arrows, etc.) -- use only plain ASCII
- Never use `from __future__ import annotations`
- Never use `__slots__`
- Use Google-style docstrings (`Args:`, `Returns:`, `Attributes:` sections) for non-trivial functions and classes
- Trivial one-liner docstrings are fine for self-explanatory functions (e.g. `read_byte`, `write_int_le`)
- Inline comments next to wrapper.passthrough/read/write calls must use the exact field names from `data/v924_packets.json` (or v944 for new fields)

## Tooling

- Always use `uv pip` instead of `pip` for package management
- Use `uv run` to execute scripts (e.g. `uv run tools/fetch_protocol_docs.py`)