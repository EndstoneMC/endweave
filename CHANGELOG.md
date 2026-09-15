# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Fixed

- Accept retail 26.50 clients on protocol 2193. The previous build supported preview protocol 2192 only, so retail clients received the Chain/outdated-server error. Retail BDS 1.26.50.5 has the same reflected packet schemas as preview 1.26.50.27 apart from the handshake protocol constraint; route 2193 through the existing 2192 codec and rewrite both handshake version fields.
- Restore the supported Minecraft version in server-list responses on older servers. The 0.5.0 rewrite removed this advertisement, leaving newer clients seeing only the server's original version before the connection handshake. Blocked client versions are excluded, including after a configuration reload.
- A 26.45 client could not join a 26.40-26.44 server, and the other way round. The two put the same bytes on the wire under different version numbers, so the server turned the number away before Endweave had anything to translate. Endweave now writes the server's number into the handshake for such a client and carries the connection untranslated.

## [0.5.0] - 2026-09-13

### Added
- Clients on 26.5x can join servers on 26.40-26.45, and the other way round. Packets are rewritten in both directions, a packet both versions read alike is passed through untouched, and a packet the other side has no counterpart for is dropped rather than forwarded. `/endweave debug` names each packet it rewrites, drops or refuses, and `debug pre`/`debug post` show the payload on either side of the rewrite.
- `/endweave list` groups the online players by the protocol version they speak. Requires `endweave.command.list`, which `endweave.admin` grants and operators hold by default.
- `/endweave debug [clear|pre|post|add|remove] [packet]` toggles debug mode, `debug pre` and `debug post` the transform logging phases, and `debug add|remove <packet>` and `debug clear` edit the packet filter. A packet name it does not know is refused rather than accepted and then silently ignored. Requires `endweave.command.debug`.
- `/endweave` on its own lists the subcommands you are allowed to run.
- `/endweave reload` reads the config files off the disk again, and reports a config file that will not parse back to whoever ran the command. Requires `endweave.command.reload`. `log-other-conversion-warnings` and `max-error-length` are the exception: the debug handler is built once when the plugin is enabled and keeps the values it was given then, so changing either still needs a restart.
- Startup reports the Minecraft versions this build translates, and warns when the server's own protocol is not one of them. Endweave would otherwise load cleanly and do nothing at all on such a server, with no indication why.
- `config.toml` is merged against the packaged defaults on startup: options added in a release show up in a config written by an older one, options that are no longer shipped are dropped, and the comments are brought back up to date. Values you have already set are kept.
- A `[logging]` section. A packet that fails to translate is always reported in the console, naming the packet, the stage it failed at, the peer and the two protocol versions; `max-error-length` caps how much of that is written, and debug mode writes it whole. `log-other-conversion-warnings` additionally warns about a packet a transform gives up on, which debug mode also turns on by itself, and `log-blocked-joins` reports every refused join.
- `block-versions` and `block-protocols` refuse a client at login, before any world data is streamed, with the kick message from `block-disconnect-msg`. The check reads the protocol the client announced in its RequestNetworkSettings, and falls back to the version the client reports on joining when that handshake was never seen, so a connection already open when the plugin was enabled is still checked. Entries that name no known version are reported on startup, and `&` colour codes in the message are translated.
- Players whose connection Endweave translates are kicked with the message from `reload-disconnect-msg` when Endweave is disabled, as it is on a reload, rather than being sent packets their version cannot read. `&` colour codes in the message are translated.

### Changed
- **BREAKING**: Packet translation now runs in a compiled C++ engine instead of in Python, and the import package is renamed from `endstone_endweave` to `endweave`. Endstone still knows the plugin as `endweave`, so `plugins/endweave/` and the config in it are kept as they are, but `import endstone_endweave` no longer works.
- **BREAKING**: Endweave now ships as one wheel per platform and Python version, where every earlier release shipped a single wheel that installed anywhere. A release carries Windows x86_64 and Linux x86_64 (glibc 2.28 or newer) builds for Python 3.10, for 3.11, and one for 3.12 and later. Alpine and other musl systems, machines that are not x86_64, and Linux distributions on an older glibc can no longer install it. The README says which file to drop in `plugins/`.
- Installing the plugin now fetches `aiohttp`, `packaging` and `tomlkit` from PyPI, so the server needs outbound network access the first time it loads Endweave.
- Minecraft 26.0 and later are named 26.x rather than 1.26.x, in `/endweave list`, the logs and the shipped config. `block-versions` accepts either form, and so does the version a joining client reports, including a preview's build number such as 1.26.50.27.
- **BREAKING**: The `[debug]` section is gone from `config.toml`, and an existing config file loses it on the first start with this release. Debug logging is turned on with `/endweave debug` and filtered with `/endweave debug add|remove <packet>` instead, and it always starts off again after a restart.

### Removed
- **BREAKING**: Translation for protocols 859 (1.21.120-1.21.123), 860 (1.21.124), 898 (1.21.130-1.21.132), 924 (26.0-26.3), 944 (26.10-26.13) and 975 (26.20). A client on one of these versions can still join a server speaking the same protocol, but it can no longer be carried to a server on a different one. These versions are not yet carried by the 0.5.0 engine.
- Failing packet payloads are no longer written to `<plugin-data>/crashes/*.bin`.
- A server-reported PacketViolationWarning is no longer surfaced as a warning in the log.

## [0.4.3] - 2026-05-08

### Fixed
- CraftingData decode crash on chemistry recipes: ShapedChemistry and ShapelessChemistry do not carry a RecipeUnlockingRequirement on the wire.
- For 1.26.20 clients on 1.26.10 servers:
    - Enchantment table showing no options.
    - Locator bar waypoints missing.
    - Environmental attribute layers (fog, sky tint, etc.) not applying.
    - Party state changes dropped.
    - Furnace, smoker, and blast furnace UIs freezing the client on open.

## [0.4.2] - 2026-05-07

### Added
- Server-reported PacketViolationWarning is now surfaced as a warning-level log so malformed-packet diagnostics are visible without enabling debug.
- Failing packet payloads are dumped to `<plugin-data>/crashes/*.bin` and the path is logged so operators can attach the file when reporting issues.

### Fixed
- ActorEvent from 1.26.20 clients carrying a trailing Fire At Position field that 1.26.10 servers reject.
- InventorySlot misreading the FullContainerName Dynamic ID as a uvarint instead of an optional uint32, which corrupted the rest of the packet and crashed 1.26.20 clients on dynamic-container interactions (e.g. bundles).

## [0.4.1] - 2026-05-06

### Fixed
- CraftingData packet failing to decode on 1.26.10 servers.
- ClientMovementPredictionSync from 1.26.20 clients carrying three new attribute floats that 1.26.10 servers reject.

## [0.4.0] - 2026-05-06

### Added
- Protocol translation for 1.26.20 (clients running 1.26.20 or later can now join 1.26.10 servers)
- Update checker that polls GitHub releases on startup and notifies operators on join (configurable via `check-for-updates` in `config.toml`)

### Fixed
- Editor mode packets causing decode errors and disconnects across mismatched 1.26.0 and 1.26.10 versions
- Volume entity spawn packets corrupted between 1.26.0 and 1.26.10 (broke fog, border, and other volume effects spawned by scripts or commands)

## [0.3.2] - 2026-04-04

### Fixed
- Block registry checksum not zeroed in some version pairs, causing clients to reject the world

## [0.3.1] - 2026-03-31

### Fixed
- Sound effects not playing correctly for 1.21.124 clients on newer servers
- Sound remapping missing for 1.21.130 clients connecting to 1.26.0 servers
- Server protocol detection failing when the server runs a Minecraft version not explicitly known to the plugin (e.g. a hotfix release like 1.26.11)

## [0.3.0] - 2026-03-30

### Added
- Protocol translation for v898 (MC 1.21.130), v860 (MC 1.21.124), and v859 (MC 1.21.120)
- Bidirectional translation (older clients can also join newer servers that have the plugin)

### Fixed
- 1.21.120/1.21.124 clients disconnecting immediately when joining 1.26.0 servers
- Animation glitches for 1.21.120/1.21.124 clients on newer servers
- Lectern page turning not working across version boundaries
- Running commands (e.g. /list) disconnecting 1.21.x clients on 1.26.0 servers
- Dismounting rides sometimes causing a disconnect
- Signs could not be edited or dyed by 1.26.10 clients on 1.26.0 servers
- Block actor interactions (e.g. editing command blocks) failing for 1.26.0 clients on 1.26.10 servers
- Script debug shapes not rendering for cross-version clients
- Client diagnostics packet causing errors when connecting across 1.21.130/1.26.0 boundary

### Changed
- Startup log now shows supported client version range instead of listing each version

## [0.2.4] - 2026-03-25

### Fixed
- Block interactions (chests, signs, etc.) failing at Y < 0 due to NetworkBlockPosition reading Y as unsigned

## [0.2.3] - 2026-03-24

### Fixed
- CameraSpline packet handler appending trailing bytes instead of per-spline fields, breaking login when `experimental_creator_cameras` is enabled
- CameraInstruction packet missing v944 spline fields (splineIdentifier, loadFromJson)

## [0.2.2] - 2026-03-23

### Fixed
- ContainerOpen packet registered as serverbound instead of clientbound, preventing v944 clients from opening chests and other containers
- bStats OS architecture not normalized across platforms

### Changed
- Dev build versions shortened

## [0.2.1] - 2026-03-21

### Fixed
- ActorData CompoundTag parsing and Int64 remapping
- bStats metrics reporting incorrect platform and plugin data

### Changed
- License changed from MIT to Apache 2.0

## [0.2.0] - 2026-03-21

### Added
- Sound event remapping so v944 clients hear the correct sounds on v924 servers
- Data-Driven UI screen packet translation (show/close screens)
- bStats metrics integration
- Improved error reporting with structured context for easier debugging
- Debug logging with packet filtering (configurable in `config.toml`)

### Changed
- Startup logs now show supported client version range

## [0.1.0] - 2026-03-20

### Added
- Protocol translation between v924 (MC 1.26.0) and v944 (MC 1.26.10)
- Automatic client version detection and protocol rewriting
- Coordinate format conversion for all affected packets
- Sound instrument remapping for note blocks
- Server list ping version spoofing so newer clients see the server
- Per-player connection tracking
- Protocol chaining support for future multi-version translation
- CI/CD with GitHub Actions

[Unreleased]: https://github.com/EndstoneMC/endweave/compare/v0.5.0...HEAD
[0.5.0]: https://github.com/EndstoneMC/endweave/compare/v0.4.3...v0.5.0
[0.4.3]: https://github.com/EndstoneMC/endweave/compare/v0.4.2...v0.4.3
[0.4.2]: https://github.com/EndstoneMC/endweave/compare/v0.4.1...v0.4.2
[0.4.1]: https://github.com/EndstoneMC/endweave/compare/v0.4.0...v0.4.1
[0.4.0]: https://github.com/EndstoneMC/endweave/compare/v0.3.2...v0.4.0
[0.3.2]: https://github.com/EndstoneMC/endweave/compare/v0.3.1...v0.3.2
[0.3.1]: https://github.com/EndstoneMC/endweave/compare/v0.3.0...v0.3.1
[0.3.0]: https://github.com/EndstoneMC/endweave/compare/v0.2.4...v0.3.0
[0.2.4]: https://github.com/EndstoneMC/endweave/compare/v0.2.3...v0.2.4
[0.2.3]: https://github.com/EndstoneMC/endweave/compare/v0.2.2...v0.2.3
[0.2.2]: https://github.com/EndstoneMC/endweave/compare/v0.2.1...v0.2.2
[0.2.1]: https://github.com/EndstoneMC/endweave/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/EndstoneMC/endweave/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/EndstoneMC/endweave/releases/tag/v0.1.0
