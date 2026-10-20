# /// script
# requires-python = ">=3.12"
# ///
"""Writes data/v<protocol>.json from one version's bedrock-dumper output.

Keeps the block_types.json and block_palette.json fields endweave reads, one entry per line and
sorted, with each state as a JSON value: a bool for a ByteTag, a number for an IntTag and a string
for a StringTag. Fields at their default are left out. From items.json it keeps the item names.

Usage: uv run tools/import_bedrock_data.py <bedrock-data-<version> folder> <protocol>
"""

import json
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


def state_value(state: dict) -> bool | int | str:
    match state["type"]:
        case "byte":
            if state["value"] not in (0, 1):
                sys.exit(f"byte state {state['name']} = {state['value']} is not a bool")
            return bool(state["value"])
        case "int" | "string":
            return state["value"]
    sys.exit(f"state {state['name']} has unknown type {state['type']}")


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
            {"name": entry["name"], "states": {s["name"]: state_value(s) for s in entry["states"]}}
            | pick(entry, BLOCK_FIELDS)
            for entry in palette["blocks"]
        ),
        key=lambda entry: (entry["name"], sorted(entry["states"].items())),
    )

    item_names = sorted(item["name"] for item in items["items"])

    def rows(entries: list) -> str:
        return ",\n".join(f"    {json.dumps(entry)}" for entry in entries)

    path = ROOT / "data" / f"v{protocol}.json"
    path.parent.mkdir(exist_ok=True)
    path.write_text(
        f'{{\n  "version": "{version}",\n'
        f'  "block_types": [\n{rows(types)}\n  ],\n'
        f'  "blocks": [\n{rows(blocks)}\n  ],\n'
        f'  "items": [\n{rows(item_names)}\n  ]\n}}\n',
        encoding="utf-8",
        newline="\n",
    )
    print(f"{path.relative_to(ROOT)}: {len(types)} block types, {len(blocks)} blocks, {len(item_names)} items")


if __name__ == "__main__":
    main()
