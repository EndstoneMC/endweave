# Endweave

> Seamlessly interweaving Bedrock protocol versions.

[![Build](https://github.com/EndstoneMC/endweave/actions/workflows/build.yml/badge.svg)](https://github.com/EndstoneMC/endweave/actions/workflows/build.yml)

An [Endstone](https://github.com/EndstoneMC/endstone) plugin that lets newer Bedrock clients connect to older servers by rewriting packets at the network layer. Inspired by [ViaVersion](https://github.com/ViaVersion/ViaVersion).

## Supported Versions

| Server Version         | Supported Clients |
|------------------------|-------------------|
| 1.26.0 (protocol 924) | 1.26.0 - 1.26.10 |

## Quick Start

1. Download the latest `.whl` from [Releases](https://github.com/EndstoneMC/endweave/releases)
2. Drop it in your server's `plugins/` folder
3. Restart the server

Players on newer clients will connect transparently. No additional configuration needed.

## How It Works

When Minecraft updates, players who update can't join servers still on the old version. Instead of waiting for an Endstone update (and keeping your community offline), Endweave lets newer clients connect right away by translating protocol differences in real time. Only fields that actually changed get rewritten, and players already on the server's version go through zero extra processing. If a client is multiple versions ahead, translators are chained together automatically.

## Contributing

Issues and PRs welcome on [GitHub](https://github.com/EndstoneMC/endweave/issues).

## License

[MIT](LICENSE)
