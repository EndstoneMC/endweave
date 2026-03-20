# Endweave

[![Build](https://github.com/EndstoneMC/endweave/actions/workflows/build.yml/badge.svg)](https://github.com/EndstoneMC/endweave/actions/workflows/build.yml)

An [Endstone](https://github.com/EndstoneMC/endstone) plugin that lets newer Bedrock clients connect to older servers by rewriting packets at the network layer. Inspired by [ViaVersion](https://github.com/ViaVersion/ViaVersion).

## Supported Versions

| Server Version         | Supported Clients  |
|------------------------|--------------------|
| 1.26.0 (protocol 924) | 1.26.0 - 1.26.10   |

## Quick Start

1. Download the latest `.whl` from [Releases](https://github.com/EndstoneMC/endweave/releases)
2. Drop it in your server's `plugins/` folder
3. Restart the server

That's it. Players on newer clients will connect transparently.

## How It Works

Bedrock packet IDs are stable across versions, so Endweave doesn't need to remap them. It only touches the packet payloads that actually changed between versions. Players already on the server's protocol version skip deserialization entirely - zero overhead for them.

Each translator module covers one version step (e.g. 924 to 944). If a client is multiple versions ahead, the pipeline chains translators automatically. New serverbound packet IDs that the older server wouldn't understand get dropped silently.

## Contributing

Issues and PRs welcome on [GitHub](https://github.com/EndstoneMC/endweave/issues).

## License

[MIT](LICENSE)
