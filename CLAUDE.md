# Endweave

ViaVersion-like protocol translation plugin for Minecraft Bedrock Edition (Endstone).
Allows newer Bedrock clients (r26_u1, protocol 944, MC 1.26.10) to connect to servers
running the current version (r26_u0, protocol 924, MC 1.26.0).

## Architecture

- **Plugin** (`plugin.py`): Endstone plugin entry point, registers event handlers
- **Pipeline** (`pipeline.py`): Routes packets through translators based on session state
- **Session Manager** (`player_state.py`): Tracks per-player protocol version
- **Codec** (`codec/`): Binary reader/writer for Bedrock packet serialization (varint, zigzag, LE/BE integers)
- **Protocol** (`protocol/`): ABC base classes + version-specific translators
- **Tools** (`tools/`): Offline scripts to fetch protocol docs, parse DOT + JSON files, generate diffs

## API Constraints (Endstone 0.11.2)

- `PacketReceiveEvent` / `PacketSendEvent`:
    - `packet_id`: **readonly** int
    - `payload`: **read/write** bytes (excludes header)
    - `cancel()`: drops the packet
- `packet_id` is readonly -- we cannot remap packet IDs (not needed: IDs are identical between 924<->944)
- Endstone fires `PacketReceiveEvent` for pre-login packets including `RequestNetworkSettings`
- `ServerListPingEvent.minecraft_version_network` is read/write

## Development

- `python tools/fetch_protocol_docs.py` -- fetch protocol DOT + JSON files from BedrockProtocol repo
- `python tools/parse_protocol_docs.py` -- parse DOT + JSON -> structured JSON
- `python tools/generate_diff.py` -- diff two version JSONs
- `pytest tests/` -- run unit tests

## Key Design Decisions

- Fast-path skip when `not session.needs_translation` (no deserialization overhead for matching clients)
- Only deserialize packets that have registered rewriters (pass-through by default)
- Rewrite `RequestNetworkSettings` protocol field to 924 so BDS accepts newer clients
- Cancel new serverbound packet IDs that the v924 server doesn't understand
