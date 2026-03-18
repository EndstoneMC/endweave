#!/usr/bin/env python3
"""Generate a diff between two protocol version JSON files.

Compares packet JSON files field-by-field to identify:
- New packets (only in one version)
- Removed packets
- Field additions/removals per packet
- Type changes

Usage:
    python tools/generate_diff.py                  # diff v924 vs v944 (default)
    python tools/generate_diff.py r26_u0 r26_u1    # diff specific versions by tag
    python tools/generate_diff.py 924 944          # diff by protocol number
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
DATA_DIR = PROJECT_ROOT / "data"

sys.path.insert(0, str(PROJECT_ROOT / "src"))
from endstone_endweave.protocol.versions import VERSIONS


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


def _resolve_protocol(arg: str) -> int | None:
    """Resolve a CLI argument to a protocol number (accepts tag or number)."""
    # Try as protocol number
    try:
        proto = int(arg)
        if proto in VERSIONS:
            return proto
    except ValueError:
        pass
    # Try as release tag
    for ver in VERSIONS.values():
        if ver.release_tag == arg:
            return ver.protocol
    return None


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Diff two protocol version packet JSONs."
    )
    parser.add_argument(
        "old",
        nargs="?",
        help="Old version (protocol number or release tag). Default: lowest known.",
    )
    parser.add_argument(
        "new",
        nargs="?",
        help="New version (protocol number or release tag). Default: highest known.",
    )
    args = parser.parse_args()

    sorted_protos = sorted(VERSIONS.keys())
    old_proto = sorted_protos[0] if len(sorted_protos) >= 1 else None
    new_proto = sorted_protos[-1] if len(sorted_protos) >= 2 else None

    if args.old:
        old_proto = _resolve_protocol(args.old)
        if old_proto is None:
            print(f"Error: unknown version '{args.old}'")
            sys.exit(1)
    if args.new:
        new_proto = _resolve_protocol(args.new)
        if new_proto is None:
            print(f"Error: unknown version '{args.new}'")
            sys.exit(1)

    if old_proto is None or new_proto is None:
        print("Error: need at least two known versions to diff.")
        sys.exit(1)

    old_path = DATA_DIR / f"v{old_proto}_packets.json"
    new_path = DATA_DIR / f"v{new_proto}_packets.json"

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

    out_path = DATA_DIR / f"v{old_proto}_v{new_proto}_diff.json"
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(diff, f, indent=2)
    print(f"  Written to {out_path}")


if __name__ == "__main__":
    main()
