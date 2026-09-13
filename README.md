# Endweave

> Seamlessly interweaving Bedrock protocol versions.

[![Build](https://github.com/EndstoneMC/endweave/actions/workflows/build.yml/badge.svg)](https://github.com/EndstoneMC/endweave/actions/workflows/build.yml)

An [Endstone](https://github.com/EndstoneMC/endstone) plugin that lets Bedrock clients connect to servers with
different protocol versions by rewriting packets at the network layer. Inspired by [ViaVersion](https://github.com/ViaVersion/ViaVersion).

## Supported Versions

Endweave translates between any two of these:

| Minecraft Version | Protocol |
| ----------------- | -------- |
| 26.40-26.44       | 2168     |
| 26.45             | 2169     |
| 26.5x             | 2192     |

### Not translated

These versions are known to Endweave but not carried by the 0.5.0 engine:

| Minecraft Version | Protocol |
| ----------------- | -------- |
| 1.21.120-1.21.123 | 859      |
| 1.21.124          | 860      |
| 1.21.130-1.21.132 | 898      |
| 26.0-26.3         | 924      |
| 26.10-26.13       | 944      |
| 26.20             | 975      |
| 26.30-26.32       | 1001     |

Endweave up to 0.4.3 translated 1.21.120 through 26.20, and 26.30 was never translated by any release. A client on
one of these can still join a server speaking the same protocol; it just gets nothing from Endweave.

## Quick Start

1. Open the latest [release](https://github.com/EndstoneMC/endweave/releases) and download the wheel that matches
   your server. Several are attached, one per platform and Python version:
    - `win_amd64` for Windows, `manylinux_2_28_x86_64` for Linux on glibc 2.28 or newer.
    - `cp310` for Python 3.10, `cp311` for 3.11, `cp312-abi3` for 3.12 and later.
2. Drop it in your server's `plugins/` folder
3. Restart the server

Endstone installs the file you drop in, so a wheel built for another platform or Python version is refused and the
plugin does not load. The install also fetches a few dependencies from PyPI, so the server needs network access the
first time it starts with Endweave in `plugins/`.

Players on newer clients will connect transparently. No additional configuration needed.

## How It Works

Endweave handles protocol differences between Minecraft versions, allowing players to join servers on different
protocol versions. Whether a client is newer or older than the server, packets are translated in real time. Only
fields that actually changed get rewritten, and players already on the server's version go through zero extra
processing. A packet the other version has no counterpart for is dropped rather than forwarded, so neither end is
handed something it cannot read.

Since 0.5.0 the translation runs in a C++ engine compiled from
[bedrock-protocol](https://github.com/EndstoneMC/bedrock-protocol) rather than in Python.

## Contributing

Issues and PRs welcome on [GitHub](https://github.com/EndstoneMC/endweave/issues).

## License

[Apache License 2.0](LICENSE)
