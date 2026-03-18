# Endweave

ViaVersion-like protocol translation plugin for Minecraft Bedrock Edition (Endstone).
Supports chained protocol translation between arbitrary Bedrock version pairs.
Currently translates between protocol 924 (MC 1.26.0) and 944 (MC 1.26.10).

## Architecture

- **Plugin** (`plugin.py`): Endstone plugin entry point, registers event handlers and translators
- **Pipeline** (`pipeline.py`): Routes packets through translator chains based on session state
- **Session Manager** (`session.py`): Tracks per-player protocol version and cached translator chain
- **Codec** (`codec/`): Binary reader/writer for Bedrock packet serialization (varint, zigzag, LE/BE integers)
- **Protocol** (`protocol/`): Translation infrastructure + version-specific translator modules
  - `versions.py`: Central `ProtocolVersion` registry (single source of truth for all known versions)
  - `packet_ids.py`: Shared `PacketId` enum (Bedrock IDs are stable across versions)
  - `registry.py`: `TranslatorRegistry` with BFS chaining for multi-step version gaps
  - `base.py`: `ProtocolTranslator` ABC and `PacketTransformation` result type
  - `v924_to_v944/`: Version-specific translator module (exports `create_translator`, `SERVER_PROTOCOL`, `CLIENT_PROTOCOL`)
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

- Fast-path skip when `not session.needs_translation` (no deserialization overhead for matching clients)
- Only deserialize packets that have registered rewriters (pass-through by default)
- Rewrite `RequestNetworkSettings` protocol field to server's version so BDS accepts newer clients
- Cancel new serverbound packet IDs that the older server doesn't understand
- Translator chaining: each module handles one adjacent version step; pipeline chains them for larger gaps
- Login handlers use `session.server_protocol` (not hardcoded constants) for version-agnostic rewriting

## Adding a New Version Pair

1. Add `R27_U0 = ProtocolVersion(960, "1.27.0", "r27_u0")` to `protocol/versions.py`
2. Run tools to fetch + parse + diff the new protocol docs
3. Create `protocol/v944_to_v960/` with:
   - `translator.py` — `SERVER_PROTOCOL = 944`, `CLIENT_PROTOCOL = 960`, `create_translator()`
   - `packet_ids.py` — re-export from shared location (or add new IDs to shared `packet_ids.py`)
   - `handlers/` — version-specific packet rewriters
4. Register in `plugin.py`: `self._register_translator(create_v944_to_v960())`

The chaining system automatically handles multi-step gaps (e.g. v960 client -> v924 server).

## Git

- Never add a Co-Authored-By line for Claude in commit messages

## Tooling

- Always use `uv pip` instead of `pip` for package management
