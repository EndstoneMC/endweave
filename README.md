# Endweave

ViaVersion-like protocol translation plugin for Minecraft Bedrock Edition, built on [Endstone](https://github.com/EndstoneMC/endstone).

Allows newer Bedrock clients to connect to servers running older protocol versions by transparently rewriting packets at the network layer. Supports chaining translators for multi-version gaps.

## How It Works

Endweave intercepts packets at the network layer and rewrites fields that differ between protocol versions. It does **not** remap packet IDs (they are stable across Bedrock versions) — instead it modifies packet payloads in-place.

- **Fast-path skip** — players on the matching protocol version have zero deserialization overhead
- **Selective rewriting** — only packets with registered translators are deserialized; everything else passes through untouched
- **Login handshake** — rewrites the `RequestNetworkSettings` protocol field so BDS accepts newer clients
- **Unknown packets** — cancels new serverbound packet IDs that the older server doesn't understand
- **Version chaining** — each translator handles one adjacent version step; the pipeline chains them automatically for larger gaps

## Architecture

```
src/endstone_endweave/
├── plugin.py          # Endstone plugin entry point
├── pipeline.py        # Routes packets through version-specific translator chains
├── session.py         # Per-player session & protocol version tracking
├── codec/             # Binary reader/writer (varint, zigzag, LE/BE integers)
├── protocol/          # Translation infrastructure + version-specific modules
│   ├── versions.py    # Central ProtocolVersion registry
│   ├── packet_ids.py  # Shared PacketId enum
│   ├── registry.py    # TranslatorRegistry with BFS chaining
│   └── v924_to_v944/  # Version-specific translator module
├── data/              # Generated protocol data (gitignored)
```

## Installation

```bash
pip install endstone-endweave
```

Or install from source for development:

```bash
git clone https://github.com/EndstoneMC/endweave.git
cd endweave
pip install -e .
```

## Development

```bash
# Fetch protocol DOT + JSON files from BedrockProtocol repo
python tools/fetch_protocol_docs.py

# Parse DOT + JSON into structured JSON
python tools/parse_protocol_docs.py

# Diff two version JSONs
python tools/generate_diff.py

# Run tests
pytest tests/
```

## License

MIT — see [LICENSE](LICENSE) for details.
