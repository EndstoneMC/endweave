# Endweave

ViaVersion-like protocol translation plugin for Minecraft Bedrock Edition, built on [Endstone](https://github.com/EndstoneMC/endstone).

Allows newer Bedrock clients (v1.26.10, protocol 944) to connect to servers still running the current stable version (v1.26.0, protocol 924).

## How It Works

Endweave intercepts packets at the network layer and rewrites fields that differ between protocol versions. It does **not** remap packet IDs (they are identical between 924 and 944) — instead it modifies packet payloads in-place.

- **Fast-path skip** — players on the matching protocol version have zero deserialization overhead
- **Selective rewriting** — only packets with registered translators are deserialized; everything else passes through untouched
- **Login handshake** — rewrites the `RequestNetworkSettings` protocol field so BDS accepts newer clients
- **Unknown packets** — cancels new serverbound packet IDs that the older server doesn't understand

## Architecture

```
src/endstone_endweave/
├── plugin.py          # Endstone plugin entry point
├── pipeline.py        # Routes packets through version-specific translators
├── player_state.py    # Per-player session & protocol version tracking
├── codec/             # Binary reader/writer (varint, zigzag, LE/BE integers)
├── protocol/          # ABC base classes + version-specific translators
└── data/              # Generated protocol data
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