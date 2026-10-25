# /// script
# requires-python = ">=3.12"
# ///
"""Writes data/v<protocol>.json from one version's bedrock-dumper output.

Keeps the block_types.json and block_palette.json fields endweave reads, one entry per line and
sorted, with each state as a JSON value: a bool for a ByteTag, a number for an IntTag and a string
for a StringTag. Fields at their default are left out. From items.json it keeps the item names, and
from biomes.json each biome's name and id.

Usage: uv run tools/import_bedrock_data.py <bedrock-data-<version> folder> <protocol>
"""

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

BLOCK_TYPE_FIELDS = {
    "default_state_id": None,
    "material_type": None,
    "signal_source": False,
    "consumer_component": False,
    "tags": [],
}
BLOCK_FIELDS = {
    "id": None,
    "flame_odds": 0,
    "connection_rule": None,
    "connection_id": None,
    "full_support_faces": 0,
    "redstone": None,
    "redstone_producer": None,
    "should_connect_to_redstone": [False] * 5,
}


SNBT_STATE = re.compile(
    r"(?:'(?P<quoted>[^']*)'|(?P<bare>[\w.+-]+)):(?:'(?P<string>[^']*)'|(?P<byte>-?\d+)b|(?P<int>-?\d+))"
)


def parse_states(snbt: str) -> dict[str, bool | int | str]:
    matches = list(SNBT_STATE.finditer(snbt))
    if "{" + ",".join(match[0] for match in matches) + "}" != snbt:
        sys.exit(f"states {snbt} are not a flat SNBT compound")
    states: dict[str, bool | int | str] = {}
    for match in matches:
        name = match["quoted"] if match["quoted"] is not None else match["bare"]
        if match["string"] is not None:
            states[name] = match["string"]
        elif match["byte"] is not None:
            if match["byte"] not in ("0", "1"):
                sys.exit(f"byte state {name} = {match['byte']} is not a bool")
            states[name] = match["byte"] == "1"
        else:
            states[name] = int(match["int"])
    return states


def pick(entry: dict, fields: dict) -> dict:
    return {key: entry[key] for key, default in fields.items() if key in entry and entry[key] != default}


def main() -> None:
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    folder, protocol = Path(sys.argv[1]), int(sys.argv[2])
    palette = json.loads((folder / "block_palette.json").read_text(encoding="utf-8"))
    block_types = json.loads((folder / "block_types.json").read_text(encoding="utf-8"))
    items = json.loads((folder / "items.json").read_text(encoding="utf-8"))
    version = folder.name.removeprefix("bedrock-data-")

    types = sorted(
        ({"name": entry["name"]} | pick(entry, BLOCK_TYPE_FIELDS) for entry in block_types["block_types"]),
        key=lambda entry: entry["name"],
    )
    blocks = sorted(
        (
            {"name": entry["name"], "states": parse_states(entry["states"])} | pick(entry, BLOCK_FIELDS)
            for entry in palette["blocks"]
        ),
        key=lambda entry: (entry["name"], sorted(entry["states"].items())),
    )

    item_names = sorted(item["name"] for item in items["items"])

    biome_list = json.loads((folder / "biomes.json").read_text(encoding="utf-8"))["biomes"]
    biomes = sorted(({"name": biome["name"], "id": biome["id"]} for biome in biome_list), key=lambda b: b["name"])

    def rows(entries: list) -> str:
        return ",\n".join(f"    {json.dumps(entry)}" for entry in entries)

    path = ROOT / "data" / f"v{protocol}.json"
    path.parent.mkdir(exist_ok=True)
    path.write_text(
        f'{{\n  "version": "{version}",\n'
        f'  "block_types": [\n{rows(types)}\n  ],\n'
        f'  "blocks": [\n{rows(blocks)}\n  ],\n'
        f'  "items": [\n{rows(item_names)}\n  ],\n'
        f'  "biomes": [\n{rows(biomes)}\n  ]\n}}\n',
        encoding="utf-8",
        newline="\n",
    )
    print(
        f"{path.relative_to(ROOT)}: {len(types)} block types, {len(blocks)} blocks, {len(item_names)} items, "
        f"{len(biomes)} biomes"
    )


if __name__ == "__main__":
    main()
