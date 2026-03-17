#!/usr/bin/env python3
"""Generate a diff between two protocol version JSON files.

Compares v924_packets.json and v944_packets.json field-by-field to identify:
- New packets (only in one version)
- Removed packets
- Field additions/removals per packet
- Type changes

Output: src/endstone_endweave/data/v924_v944_diff.json
"""

from __future__ import annotations

import json
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
DATA_DIR = PROJECT_ROOT / "src" / "endstone_endweave" / "data"


def flatten_fields(fields: list[dict], prefix: str = "") -> dict[str, dict]:
    """Flatten a field tree into a dict keyed by dotted path."""
    result = {}
    for field in fields:
        name = field["name"]
        path = f"{prefix}.{name}" if prefix else name
        result[path] = {
            "type": field.get("type", ""),
            "attributes": field.get("attributes", 0),
        }
        if "children" in field:
            result.update(flatten_fields(field["children"], path))
    return result


def diff_packets(old_packets: list[dict], new_packets: list[dict]) -> dict:
    """Diff two lists of packet definitions."""
    old_by_name = {p["name"]: p for p in old_packets}
    new_by_name = {p["name"]: p for p in new_packets}

    old_names = set(old_by_name.keys())
    new_names = set(new_by_name.keys())

    result = {
        "new_packets": sorted(new_names - old_names),
        "removed_packets": sorted(old_names - new_names),
        "changed_packets": {},
    }

    # Compare packets present in both versions
    for name in sorted(old_names & new_names):
        old_fields = flatten_fields(old_by_name[name].get("fields", []))
        new_fields = flatten_fields(new_by_name[name].get("fields", []))

        old_field_names = set(old_fields.keys())
        new_field_names = set(new_fields.keys())

        added = sorted(new_field_names - old_field_names)
        removed = sorted(old_field_names - new_field_names)

        type_changes = {}
        for field_name in sorted(old_field_names & new_field_names):
            old_type = old_fields[field_name]["type"]
            new_type = new_fields[field_name]["type"]
            if old_type != new_type:
                type_changes[field_name] = {"old": old_type, "new": new_type}

        if added or removed or type_changes:
            result["changed_packets"][name] = {
                "added_fields": added,
                "removed_fields": removed,
                "type_changes": type_changes,
            }

    return result


def main() -> None:
    old_path = DATA_DIR / "v924_packets.json"
    new_path = DATA_DIR / "v944_packets.json"

    if not old_path.exists() or not new_path.exists():
        print("Error: Run parse_protocol_docs.py first to generate packet JSON files.")
        print(f"  Expected: {old_path}")
        print(f"  Expected: {new_path}")
        return

    with open(old_path, encoding="utf-8") as f:
        old_packets = json.load(f)
    with open(new_path, encoding="utf-8") as f:
        new_packets = json.load(f)

    print(f"Comparing {len(old_packets)} old packets vs {len(new_packets)} new packets...")
    diff = diff_packets(old_packets, new_packets)

    print(f"  New packets: {len(diff['new_packets'])}")
    print(f"  Removed packets: {len(diff['removed_packets'])}")
    print(f"  Changed packets: {len(diff['changed_packets'])}")

    out_path = DATA_DIR / "v924_v944_diff.json"
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(diff, f, indent=2)
    print(f"  Written to {out_path}")


if __name__ == "__main__":
    main()
