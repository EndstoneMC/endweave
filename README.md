# Endweave

> Seamlessly interweaving Bedrock protocol versions.

[![CI](https://github.com/EndstoneMC/endweave/actions/workflows/ci.yml/badge.svg)](https://github.com/EndstoneMC/endweave/actions/workflows/ci.yml)

An [Endstone](https://github.com/EndstoneMC/endstone) plugin that lets Bedrock clients connect to servers with
different protocol versions by rewriting packets at the network layer. Inspired by [ViaVersion](https://github.com/ViaVersion/ViaVersion).

> [!NOTE]
> The published [Releases](https://github.com/EndstoneMC/endweave/releases) are the earlier Python implementation.
> This branch is a C++ rewrite that has not been released yet; build it from source until it has.

## Supported Versions

| Minecraft Version | Protocol |
|-------------------|----------|
| 1.26.30           | 1001     |
| 1.26.40           | 2168     |
| 1.26.50           | 2181     |

A client on any of these can join a server on any other. A client the plugin does not know is left alone and
meets whatever the server would have told it anyway.

## Quick Start

1. Download the latest plugin from [Releases](https://github.com/EndstoneMC/endweave/releases)
2. Drop it in your server's `plugins/` folder
3. Restart the server

Players on other versions will connect transparently. No additional configuration needed. A `config.toml` with
debug logging turned off is written into the plugin's data folder on first run.

## Building from Source

Requires CMake, Ninja, and Endstone's toolchain floor: Clang 18+ with libc++ on Linux, clang-cl on Windows. The
wire codec comes from [bedrock-protocol](https://github.com/EndstoneMC/bedrock-protocol), which is read from a
checkout beside this one.

```shell
git clone https://github.com/EndstoneMC/bedrock-protocol.git
git clone https://github.com/EndstoneMC/endweave.git
cd endweave
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake --build build
```

Pass `-DBEDROCK_PROTOCOL_SOURCE_DIR=<path>` if that checkout lives somewhere else. The plugin lands in `build/`
as `endweave-<version>.so` or `.dll`.

## How It Works

Endweave handles protocol differences between Minecraft versions, allowing players to join servers on different
protocol versions. Whether a client is newer or older than the server, packets are translated in real time. Only
packets that actually changed between the two versions get rewritten, and players already on the server's version
go through zero extra processing.

## Contributing

Issues and PRs welcome on [GitHub](https://github.com/EndstoneMC/endweave/issues).

## License

[Apache License 2.0](LICENSE)
