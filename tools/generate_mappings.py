# /// script
# requires-python = ">=3.12"
# ///
"""Generates each hop's mappings from the block palettes and item names in data/.

Each hop gets a mappings.h: its block_states, and its items if the target lacks any of the source's.
The classes mirror BDS's own, so a block's network id is computed the way
Block::computeRawSerializationIdHashForNetwork computes it. Going up, a block gains the states the
newer version added, at that version's default, and loses the ones it removed. Going down, it
loses what the older version lacks, and a block type the older version lacks is renamed to the
fallback data/fallbacks/v<from>_to_v<to>.json names for it. Anything else stops the generator.
items lists, sorted, the items the target has no mapping for. A hop that gives blocks states from
their neighbours also gets a block_connections.h, a BlockData profile for each set of blocks that
are the same to the blocks beside them.

Usage: uv run tools/generate_mappings.py
"""

import itertools
import json
import math
import re
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "data"
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


def to_tag(value: bool | int | str) -> Tag:
    if isinstance(value, bool):
        return ByteTag(int(value))
    if isinstance(value, int):
        return IntTag(value)
    return StringTag(value)


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
    """One version's blocks, and its item names, read from data/v<protocol>.json."""

    def __init__(self, protocol: int, mapping: dict) -> None:
        self.protocol = protocol
        self.version = mapping["version"]
        self.items = set(mapping["items"])
        self.blocks: dict[tuple, Block] = {}
        # Every value each state of each block type takes.
        self.states: dict[str, dict[str, list[Tag]]] = {}
        # What each block is to the blocks beside it, in the order BlockData declares it.
        self.connection_data: dict[tuple, tuple] = {}
        block_types = {block_type["name"]: block_type for block_type in mapping["block_types"]}
        by_runtime_id: dict[int, Block] = {}
        for entry in mapping["blocks"]:
            block = Block(entry["name"], {name: to_tag(value) for name, value in entry["states"].items()})
            if block.get_runtime_id() != entry["id"] & 0xFFFFFFFF:
                sys.exit(f"{block} does not hash to its id {entry['id']}")
            self.blocks[block.key()] = block
            rule = entry.get("connection_rule", {})
            block_type = block_types[block.name]
            self.connection_data[block.key()] = (
                rule.get("connections_from"),
                rule.get("enabled_directions", 0),
                entry.get("connection_id"),
                entry.get("flame_odds", 0),
                entry.get("redstone", {}).get("redstone_conductor", False),
                entry.get("redstone_producer", {}).get("connected_faces"),
                sum(1 << i for i, connects in enumerate(entry.get("should_connect_to_redstone", [])) if connects),
                entry.get("full_support_faces", 0),
                block_type.get("signal_source", False),
                block_type.get("consumer_component", False),
            )
            by_runtime_id[block.get_runtime_id()] = block
            values = self.states.setdefault(block.name, {})
            for name, tag in block.states.items():
                if tag not in values.setdefault(name, []):
                    values[name].append(tag)
        for values in self.states.values():
            for tags in values.values():
                tags.sort()
        self.defaults = {}
        for block_type in mapping["block_types"]:
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
    if unused := sorted(name for name in fallbacks if name not in source.states or name in target.states):
        sys.exit(
            f"data/fallbacks/v{source.protocol}_to_v{target.protocol}.json names a fallback for {', '.join(unused)}, "
            f"which {target.version} has or {source.version} lacks"
        )
    updates = []
    for name in sorted(source.states):
        new_name = name if name in target.states else fallbacks.get(name)
        if new_name is None:
            sys.exit(
                f"{name} is not in {target.version}: add a fallback for it to "
                f"data/fallbacks/v{source.protocol}_to_v{target.protocol}.json"
            )
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


# BlockData's fields after its name and states, and how each is written when it is not its default.
BLOCK_DATA_FIELDS = {
    "connections_from": (None, "0b{:05b}"),
    "enabled_directions": (0, "0b{:06b}"),
    "connection_id": (None, "{}"),
    "flame_odds": (0, "{}"),
    "redstone_conductor": (False, "true"),
    "redstone_producer_faces": (None, "0b{:06b}"),
    "should_connect_to_redstone": (0, "0b{:05b}"),
    "full_support_faces": (0, "0b{:06b}"),
    "signal_source": (False, "true"),
    "consumer_component": (False, "true"),
}


@dataclass
class BlockData:
    """The blocks of one type that are the same to the blocks beside them: one profile."""

    name: str
    states: dict[str, list[Tag]]
    fields: tuple


def products(blocks: list[dict[str, Tag]]) -> list[dict[str, list[Tag]]]:
    """Splits blocks of one type into sets that are each every combination of some state values."""
    values = {name: sorted({block[name] for block in blocks}) for name in sorted(blocks[0])}
    if len(blocks) == math.prod(len(tags) for tags in values.values()):
        return [values]
    split = next(name for name, tags in values.items() if len(tags) > 1)
    return [states for tag in values[split] for states in products([b for b in blocks if b[split] == tag])]


def block_connections(
    source: BlockPalette, target: BlockPalette, updates: list[BlockStateUpdate]
) -> list[BlockData] | None:
    """What every block `source` can send is to the blocks beside it, as `target` has it, for a hop
    that gives blocks states."""
    if not any(update.add for update in updates):
        return None
    blocks = []
    for name in sorted(source.states.keys() & target.states.keys()):
        groups: dict[tuple, list[dict[str, Tag]]] = {}
        for block in target.permutations(name):
            groups.setdefault(target.connection_data[block.key()], []).append(block.states)
        for fields, states in sorted(groups.items(), key=lambda group: repr(group[0])):
            blocks += [BlockData(name, product, fields) for product in products(states)]
    return blocks


def snbt_string(value: str) -> str:
    if any(c in value for c in "'\"\\"):
        sys.exit(f"{value} needs escaping in SNBT")
    return f"'{value}'"


def snbt_key(name: str) -> str:
    return name if re.fullmatch(r"[0-9A-Za-z_\-.+]+", name) else snbt_string(name)


def snbt(tag: Tag) -> str:
    if isinstance(tag, ByteTag):
        return f"{tag.data}b"
    if isinstance(tag, IntTag):
        return str(tag.data)
    return snbt_string(tag.data)


def cpp_snbt(field: str, value: str) -> list[str]:
    return [f'            .{field} = "{value}",']


def cpp_states(states: dict[str, list[Tag]]) -> list[str]:
    if not states:
        return []
    values = (f"{snbt_key(state)}: [{', '.join(snbt(tag) for tag in tags)}]" for state, tags in sorted(states.items()))
    return cpp_snbt("states", f"{{{', '.join(values)}}}")


def cpp_block_data(block: BlockData) -> list[str]:
    lines = ["        {", f'            .name = "{block.name}",', *cpp_states(block.states)]
    for (member, (default, form)), value in zip(BLOCK_DATA_FIELDS.items(), block.fields):
        if value != default:
            lines.append(f"            .{member} = {form.format(value)},")
    lines.append("        },")
    return lines


def cpp_update(update: BlockStateUpdate) -> list[str]:
    lines = ["        {", f'            .name = "{update.name}",', *cpp_states(update.states)]
    if update.rename:
        lines.append(f'            .rename = "{update.rename}",')
    if update.add:
        values = ", ".join(f"{snbt_key(state)}: {snbt(tag)}" for state, tag in sorted(update.add.items()))
        lines += cpp_snbt("add", f"{{{values}}}")
    if update.remove:
        lines += cpp_snbt("remove", f"[{', '.join(snbt_string(state) for state in update.remove)}]")
    lines.append("        },")
    return lines


def preamble(source: BlockPalette, target: BlockPalette, header: str) -> list[str]:
    return [
        "// clang-format off",
        f"// Generated by tools/generate_mappings.py from data/v{source.protocol}.json ({source.version}) and",
        f"// data/v{target.protocol}.json ({target.version}).",
        "",
        "#pragma once",
        "",
        f'#include "endweave/protocol/{header}"',
        "",
    ]


def write_lines(source: BlockPalette, target: BlockPalette, file: str, lines: list[str]) -> Path:
    path = PROTOCOLS / f"v{source.protocol}_to_v{target.protocol}" / file
    path.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    return path


def write_mappings(
    source: BlockPalette, target: BlockPalette, updates: list[list[str]], missing_items: list[str]
) -> None:
    lines = [
        *preamble(source, target, "mapping.h"),
        "#include <array>",
        "#include <string_view>",
        "",
        "namespace endweave {",
        "",
        "template <>",
        f"struct Mappings<{source.protocol}, {target.protocol}> {{",
        f"    static constexpr std::array<BlockStateUpdate, {len(updates)}> block_states = {{{{",
        *(line for update in updates for line in update),
        "    }};",
    ]
    if missing_items:
        lines += [
            f"    static constexpr std::array<std::string_view, {len(missing_items)}> items = {{{{",
            *(f'        "{name}",' for name in missing_items),
            "    }};",
        ]
    lines += ["};", "", "} // namespace endweave", ""]
    path = write_lines(source, target, "mappings.h", lines)
    print(f"{path.relative_to(ROOT)}: {len(updates)} block states, {len(missing_items)} items")


def write_block_connections(source: BlockPalette, target: BlockPalette, profiles: list[list[str]]) -> None:
    lines = [
        *preamble(source, target, "block_connection.h"),
        "#include <array>",
        "",
        "namespace endweave {",
        "",
        "template <>",
        f"struct BlockConnections<{source.protocol}, {target.protocol}> {{",
        f"    static constexpr std::array<BlockData, {len(profiles)}> profiles = {{{{",
        *(line for profile in profiles for line in profile),
        "    }};",
        "};",
        "",
        "} // namespace endweave",
        "",
    ]
    path = write_lines(source, target, "block_connections.h", lines)
    print(f"{path.relative_to(ROOT)}: {len(profiles)} profiles")


def write_hop(source: BlockPalette, target: BlockPalette, fallbacks: dict[str, str]) -> None:
    updates = diff(source, target, fallbacks)
    write_mappings(source, target, [cpp_update(update) for update in updates], sorted(source.items - target.items))
    if (profiles := block_connections(source, target, updates)) is not None:
        write_block_connections(source, target, [cpp_block_data(profile) for profile in profiles])


def load_fallbacks(source: BlockPalette, target: BlockPalette) -> dict[str, str]:
    path = DATA / "fallbacks" / f"v{source.protocol}_to_v{target.protocol}.json"
    if not path.exists():
        return {}
    return json.loads(path.read_text(encoding="utf-8")).get("block_types", {})


def main() -> None:
    if len(sys.argv) != 1:
        sys.exit(__doc__)
    palettes = sorted(
        (
            BlockPalette(int(path.stem.removeprefix("v")), json.loads(path.read_text(encoding="utf-8")))
            for path in DATA.glob("v*.json")
        ),
        key=lambda palette: palette.protocol,
    )
    for older, newer in zip(palettes, palettes[1:]):
        write_hop(older, newer, load_fallbacks(older, newer))
        write_hop(newer, older, load_fallbacks(newer, older))


if __name__ == "__main__":
    main()
