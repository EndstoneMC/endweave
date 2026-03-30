# Endweave

> Seamlessly interweaving Bedrock protocol versions.

[![Build](https://github.com/EndstoneMC/endweave/actions/workflows/build.yml/badge.svg)](https://github.com/EndstoneMC/endweave/actions/workflows/build.yml)

An [Endstone](https://github.com/EndstoneMC/endstone) plugin that lets Bedrock clients connect to servers with
different protocol versions by rewriting packets at the network layer. Inspired by [ViaVersion](https://github.com/ViaVersion/ViaVersion).

## Supported Versions

| Minecraft Version   | Protocol |
|---------------------|----------|
| 1.21.120            | 859      |
| 1.21.124            | 860      |
| 1.21.130 - 1.21.132 | 898      |
| 1.26.0 - 1.26.3     | 924      |
| 1.26.10             | 944      |

## Quick Start

1. Download the latest `.whl` from [Releases](https://github.com/EndstoneMC/endweave/releases)
2. Drop it in your server's `plugins/` folder
3. Restart the server

Players on newer clients will connect transparently. No additional configuration needed.

## How It Works

Endweave handles protocol differences between Minecraft versions, allowing players to join servers on different protocol
versions. Whether a client is newer or older than the server, packets are translated in real time. Only fields that
actually changed get rewritten, and players already on the server's version go through zero extra processing. When a
client is multiple versions away from the server, translators are automatically chained together to bridge the gap.

## Contributing

Issues and PRs welcome on [GitHub](https://github.com/EndstoneMC/endweave/issues).

## License

[Apache License 2.0](LICENSE)
