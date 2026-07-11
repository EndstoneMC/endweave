# endweave

A protocol-translation plugin for [Endstone](https://github.com/EndstoneMC/endstone).
endweave lets clients on an older Minecraft: Bedrock protocol join a server
running a newer one (and vice versa) by translating packets on the wire.

This branch is a **target-driven MVP**: rather than aiming for full-protocol
coverage, it implements the wire diff one protocol step at a time, driven by the
packets that actually change. The first slice bridges protocol **975**
(1.26.20) and **1001** — the "976 step" that added three fields to
`ClientboundAttributeLayerSyncPacket`.

The wire codec lives in the sibling [bedrock-protocol](https://github.com/EndstoneMC/bedrock-protocol)
library (a Python DSL that generates per-version C++ packet types). endweave
owns the *translation* semantics: what to do when a field is added, removed, or
reshaped between versions.

## Layout

- `include/endweave/protocol/attribute_layer_sync.h` — `upgrade`/`downgrade`
  between the 975 and 1001 forms of the packet.
- `src/protocol/attribute_layer_sync.cpp` — the field-by-field converters.
- `tests/` — bidirectional translation tests driven by per-version goldens.
- `src/plugin.cpp`, `include/endweave/plugin.h` — the Endstone plugin shell (the
  receive → translate → emit pipeline is a stub in this MVP).

## Building

endweave consumes bedrock-protocol from a sibling checkout by default
(`../bedrock-protocol`).

```shell
# Translation library + tests only (no Endstone SDK fetch):
cmake -B build -DENDWEAVE_BUILD_PLUGIN=OFF
cmake --build build
ctest --test-dir build --output-on-failure

# Full build including the Endstone plugin:
cmake -B build
cmake --build build
```
