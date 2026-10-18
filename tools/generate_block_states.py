# /// script
# requires-python = ">=3.12"
# ///
"""Generates each hop's block state updates from two of bedrock-dumper's block palettes.

The classes mirror BDS's own, so a block's network id is computed the way
Block::computeRawSerializationIdHashForNetwork computes it. Going up, a block gains the states the
newer version added, at that version's default, and loses the ones it removed. Going down, it
loses what the older version lacks, and a block type the older version lacks is renamed to the
fallback tools/blocks.toml names for it. Anything else stops the generator.

Usage: uv run tools/generate_block_states.py <dir holding bedrock-data-<version>/ folders>
"""

import hashlib
import itertools
import json
import struct
import sys
import tomllib
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PROTOCOLS = ROOT / "src" / "endweave" / "protocols"

UNKNOWN_BLOCK_NETWORKID = 4294967294


class StringByteOutput:
    """IDataOutput writing little-endian into a string."""

    def __init__(self) -> None:
        self.buffer = bytearray()

    def write_byte(self, value: int) -> None:
        self.buffer += struct.pack("<b", value)

    def write_short(self, value: int) -> None:
        self.buffer += struct.pack("<h", value)

    def write_int(self, value: int) -> None:
        self.buffer += struct.pack("<i", value)

    def write_string(self, value: str) -> None:
        data = value.encode()
        self.write_short(len(data))
        self.buffer += data


class Tag:
    END, BYTE, INT, STRING, COMPOUND = 0, 1, 3, 8, 10

    def get_id(self) -> int:
        raise NotImplementedError

    def write(self, dos: StringByteOutput) -> None:
        raise NotImplementedError


@dataclass(frozen=True, order=True)
class ByteTag(Tag):
    data: int

    def get_id(self) -> int:
        return Tag.BYTE

    def write(self, dos: StringByteOutput) -> None:
        dos.write_byte(self.data)


@dataclass(frozen=True, order=True)
class IntTag(Tag):
    data: int

    def get_id(self) -> int:
        return Tag.INT

    def write(self, dos: StringByteOutput) -> None:
        dos.write_int(self.data)


@dataclass(frozen=True, order=True)
class StringTag(Tag):
    data: str

    def get_id(self) -> int:
        return Tag.STRING

    def write(self, dos: StringByteOutput) -> None:
        dos.write_string(self.data)


@dataclass
class CompoundTag(Tag):
    tags: dict[str, Tag] = field(default_factory=dict)

    def get_id(self) -> int:
        return Tag.COMPOUND

    def get(self, name: str) -> Tag | None:
        return self.tags.get(name)

    def put(self, name: str, tag: Tag) -> None:
        self.tags[name] = tag

    def write(self, dos: StringByteOutput) -> None:
        # A std::map, so the entries go out in key order.
        for name in sorted(self.tags):
            tag = self.tags[name]
            dos.write_byte(tag.get_id())
            dos.write_string(name)
            tag.write(dos)
        dos.write_byte(Tag.END)


class NbtIo:
    @staticmethod
    def write(tag: CompoundTag, dos: StringByteOutput) -> None:
        dos.write_byte(tag.get_id())
        dos.write_string("")
        tag.write(dos)


TAG_TYPES = {"byte": ByteTag, "int": IntTag, "string": StringTag}


@dataclass
class Block:
    """One block state: its type's name and the value of each of its states."""

    name: str
    states: dict[str, Tag]

    @property
    def serialization_id(self) -> CompoundTag:
        return CompoundTag({"name": StringTag(self.name), "states": CompoundTag(dict(self.states))})

    def compute_raw_serialization_id_hash_for_network(self) -> int:
        tag = CompoundTag()
        for name in ("name", "states"):
            if (value := self.serialization_id.get(name)) is not None:
                tag.put(name, value)
        stream = StringByteOutput()
        NbtIo.write(tag, stream)
        result = 0x811C9DC5
        for byte in stream.buffer:
            result = ((result ^ byte) * 0x01000193) & 0xFFFFFFFF
        return result

    def get_runtime_id(self) -> int:
        if self.name == "minecraft:unknown":
            return UNKNOWN_BLOCK_NETWORKID
        return self.compute_raw_serialization_id_hash_for_network()

    def key(self) -> tuple:
        return self.name, tuple(sorted(self.states.items()))


class BlockPalette:
    """One version's blocks, read from bedrock-dumper's block_palette.json and block_types.json."""

    def __init__(self, protocol: int, version: str, palette: dict, block_types: dict) -> None:
        self.protocol = protocol
        self.version = version
        self.blocks: dict[tuple, Block] = {}
        # Every value each state of each block type takes.
        self.states: dict[str, dict[str, list[Tag]]] = {}
        by_runtime_id: dict[int, Block] = {}
        for entry in palette["blocks"]:
            block = Block(entry["name"], {s["name"]: TAG_TYPES[s["type"]](s["value"]) for s in entry["states"]})
            if block.get_runtime_id() != entry["id"] & 0xFFFFFFFF:
                sys.exit(f"{block} does not hash to its id {entry['id']}")
            self.blocks[block.key()] = block
            by_runtime_id[block.get_runtime_id()] = block
            values = self.states.setdefault(block.name, {})
            for name, tag in block.states.items():
                if tag not in values.setdefault(name, []):
                    values[name].append(tag)
        for values in self.states.values():
            for tags in values.values():
                tags.sort()
        self.defaults = {}
        for block_type in block_types["block_types"]:
            self.defaults[block_type["name"]] = by_runtime_id[block_type["default_state_id"] & 0xFFFFFFFF]

    def permutations(self, name: str):
        names = sorted(self.states[name])
        for values in itertools.product(*(self.states[name][n] for n in names)):
            yield Block(name, dict(zip(names, values)))


@dataclass
class BlockStateUpdate:
    """How one block type at the older or newer version becomes the other version's."""

    name: str
    states: dict[str, list[Tag]]
    rename: str | None = None
    add: dict[str, Tag] = field(default_factory=dict)
    remove: list[str] = field(default_factory=list)

    def apply(self, block: Block) -> Block:
        states = dict(block.states) | self.add
        for name in self.remove:
            del states[name]
        return Block(self.rename or block.name, states)


def diff(source: BlockPalette, target: BlockPalette, fallbacks: dict[str, str]) -> list[BlockStateUpdate]:
    updates = []
    for name in sorted(source.states):
        new_name = name if name in target.states else fallbacks.get(name)
        if new_name is None:
            sys.exit(f"{name} is not in {target.version}: add a fallback for it to tools/blocks.toml")
        states, new_states = source.states[name], target.states[new_name]
        update = BlockStateUpdate(
            name,
            states,
            rename=new_name if new_name != name else None,
            add={s: target.defaults[new_name].states[s] for s in sorted(new_states) if s not in states},
            remove=sorted(s for s in states if s not in new_states),
        )
        for state in states.keys() & new_states.keys():
            if not set(states[state]) <= set(new_states[state]):
                sys.exit(f"{name} state {state} changes its values between {source.version} and {target.version}")
        if update.rename or update.add or update.remove:
            updates.append(update)
    for update in updates:
        for block in source.permutations(update.name):
            if update.apply(block).key() not in target.blocks:
                sys.exit(f"{block} has no state at {target.version}")
    return updates


def cpp_tag(tag: Tag) -> str:
    if isinstance(tag, StringTag):
        return f'bp::StringTag{{"{tag.data}"}}'
    return f"bp::{type(tag).__name__}{{{tag.data}}}"


def cpp_update(update: BlockStateUpdate) -> list[str]:
    lines = ["        {", f'            .name = "{update.name}",']
    if update.states:
        lines.append("            .states = {")
        for state, tags in sorted(update.states.items()):
            lines.append(f'                {{"{state}", {{{", ".join(cpp_tag(tag) for tag in tags)}}}}},')
        lines.append("            },")
    if update.rename:
        lines.append(f'            .rename = "{update.rename}",')
    if update.add:
        lines.append("            .add = {")
        lines += [f'                {{"{state}", {cpp_tag(tag)}}},' for state, tag in update.add.items()]
        lines.append("            },")
    if update.remove:
        lines.append("            .remove = {")
        lines += [f'                "{state}",' for state in update.remove]
        lines.append("            },")
    lines.append("        },")
    return lines


def write(source: BlockPalette, target: BlockPalette, updates: list[BlockStateUpdate]) -> None:
    path = PROTOCOLS / f"v{source.protocol}_to_v{target.protocol}" / "block_states.h"
    lines = [
        "// clang-format off",
        f"// Generated by tools/generate_block_states.py from bedrock-dumper's {source.version} and {target.version}",
        "// block palettes.",
        "",
        "#pragma once",
        "",
        '#include "endweave/protocol/block.h"',
        "",
        "#include <vector>",
        "",
        "namespace endweave {",
        "",
        "template <>",
        f"inline std::vector<BlockStateUpdate> getBlockStateUpdates<{source.protocol}, {target.protocol}>()",
        "{",
        "    return {",
        *(line for update in updates for line in cpp_update(update)),
        "    };",
        "}",
        "",
        "} // namespace endweave",
        "",
    ]
    path.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print(f"{path.relative_to(ROOT)}: {len(updates)} block types")


def main() -> None:
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    config = tomllib.loads((ROOT / "tools" / "blocks.toml").read_text(encoding="utf-8"))
    palettes = []
    for protocol, pin in sorted(config["palettes"].items(), key=lambda item: int(item[0])):
        folder = Path(sys.argv[1]) / f"bedrock-data-{pin['version']}"
        documents = []
        for file, digest in (("block_palette.json", pin["palette"]), ("block_types.json", pin["types"])):
            data = (folder / file).read_bytes()
            if hashlib.sha256(data).hexdigest() != digest:
                sys.exit(f"{folder / file} does not match its sha256 in tools/blocks.toml")
            documents.append(json.loads(data))
        palettes.append(BlockPalette(int(protocol), pin["version"], *documents))

    fallbacks = config.get("fallbacks", {})
    for older, newer in zip(palettes, palettes[1:]):
        write(older, newer, diff(older, newer, {}))
        write(newer, older, diff(newer, older, fallbacks.get(str(newer.protocol), {})))


if __name__ == "__main__":
    main()
