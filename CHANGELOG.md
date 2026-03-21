# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Fixed
- bStats OS architecture not normalized across platforms

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
