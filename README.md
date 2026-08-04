# Endweave

> Seamlessly interweaving Bedrock protocol versions.

[![CI](https://github.com/EndstoneMC/endweave/actions/workflows/ci.yml/badge.svg)](https://github.com/EndstoneMC/endweave/actions/workflows/ci.yml)

An [Endstone](https://github.com/EndstoneMC/endstone) plugin that lets Bedrock clients connect to servers with
different protocol versions by rewriting packets at the network layer. Inspired by [ViaVersion](https://github.com/ViaVersion/ViaVersion).

> [!WARNING]
> This branch is a rewrite in progress. No packets are translated yet and there is nothing to install.
> Watch [Releases](https://github.com/EndstoneMC/endweave/releases) for the first usable build.

## Supported Versions

| Minecraft Version | Protocol |
|-------------------|----------|
| 1.26.30           | 1001     |
| 1.26.40           | 2168     |

## Quick Start

1. Download the latest plugin from [Releases](https://github.com/EndstoneMC/endweave/releases)
2. Drop it in your server's `plugins/` folder
3. Restart the server

Players on other versions will connect transparently. No additional configuration needed.

## How It Works

Endweave handles protocol differences between Minecraft versions, allowing players to join servers on different
protocol versions. Whether a client is newer or older than the server, packets are translated in real time. Only
packets that actually changed between the two versions get rewritten, and players already on the server's version
go through zero extra processing.

## Contributing

Issues and PRs welcome on [GitHub](https://github.com/EndstoneMC/endweave/issues).

## License

[Apache License 2.0](LICENSE)
