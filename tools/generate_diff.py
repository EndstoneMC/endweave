#!/usr/bin/env python3
"""Generate a diff between two protocol version JSON files.

Compares packet JSON files field-by-field to identify:
- New packets and types (separated by whether they have a packet_id)
- Removed packets and types
- Field additions/removals per packet (filtered for DOT node ID noise)
- Type changes
- Packet metadata (packet_id, direction)

Usage:
    uv run tools/generate_diff.py                  # diff lowest vs highest known
    uv run tools/generate_diff.py r26_u0 r26_u1    # diff specific versions by tag
    uv run tools/generate_diff.py 924 944           # diff by protocol number
"""

import argparse
import json
import sys
from pathlib import Path

from models import (
    PacketChanges,
    PacketDefinition,
    ProtocolDiff,
    TypeChange,
    is_node_id_noise,
)

PROJECT_ROOT = Path(__file__).resolve().parent.parent
DATA_DIR = PROJECT_ROOT / "data"

sys.path.insert(0, str(PROJECT_ROOT / "src"))
from endstone_endweave.protocol.versions import VERSIONS  # noqa: E402


def flatten_fields(fields: list[dict], prefix: str = "") -> dict[str, dict]:
    """Flatten a field tree into a dict keyed by dotted path.

    Args:
        fields: List of field dicts, each with "name", "type", and optional
            "children".
        prefix: Dotted path prefix for recursion (empty at the top level).

    Returns:
        Dict mapping dotted field paths to their type and attributes info.
    """
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


def diff_packets(
    old_packets: list[PacketDefinition],
    new_packets: list[PacketDefinition],
    old_protocol: int,
    new_protocol: int,
) -> ProtocolDiff:
    """Diff two lists of packet definitions.

    Compares packets by name, identifying additions, removals, and per-packet
    field changes. Separates actual packets (with packet_id) from sub-types.
    Filters out DOT node ID renumbering noise.

    Args:
        old_packets: Definitions from the older protocol version.
        new_packets: Definitions from the newer protocol version.
        old_protocol: Protocol number of the older version.
        new_protocol: Protocol number of the newer version.

    Returns:
        ProtocolDiff summarizing all differences.
    """
    old_by_name = {p.name: p for p in old_packets}
    new_by_name = {p.name: p for p in new_packets}

    old_names = set(old_by_name.keys())
    new_names = set(new_by_name.keys())

    added_names = sorted(new_names - old_names)
    removed_names = sorted(old_names - new_names)

    # Separate packets (have packet_id) from sub-types
    new_pkt_names = [n for n in added_names if new_by_name[n].packet_id is not None]
    new_type_names = [n for n in added_names if new_by_name[n].packet_id is None]
    removed_pkt_names = [n for n in removed_names if old_by_name[n].packet_id is not None]
    removed_type_names = [n for n in removed_names if old_by_name[n].packet_id is None]

    changed: dict[str, PacketChanges] = {}

    for name in sorted(old_names & new_names):
        old_def = old_by_name[name]
        new_def = new_by_name[name]

        old_fields = flatten_fields(
            [f.model_dump(exclude_defaults=True) for f in old_def.fields]
        )
        new_fields = flatten_fields(
            [f.model_dump(exclude_defaults=True) for f in new_def.fields]
        )

        old_field_names = set(old_fields.keys())
        new_field_names = set(new_fields.keys())

        # Filter out DOT node ID noise
        added = sorted(
            f for f in (new_field_names - old_field_names) if not is_node_id_noise(f)
        )
        removed = sorted(
            f for f in (old_field_names - new_field_names) if not is_node_id_noise(f)
        )

        type_changes: dict[str, TypeChange] = {}
        for field_name in sorted(old_field_names & new_field_names):
            old_type = old_fields[field_name]["type"]
            new_type = new_fields[field_name]["type"]
            if old_type != new_type:
                type_changes[field_name] = TypeChange(old=old_type, new=new_type)

        if added or removed or type_changes:
            changed[name] = PacketChanges(
                packet_id=old_def.packet_id,
                direction=old_def.direction,
                added_fields=added,
                removed_fields=removed,
                type_changes=type_changes,
            )

    return ProtocolDiff(
        old_protocol=old_protocol,
        new_protocol=new_protocol,
        new_packets=new_pkt_names,
        new_types=new_type_names,
        removed_packets=removed_pkt_names,
        removed_types=removed_type_names,
        changed_packets=changed,
    )


def _resolve_protocol(arg: str) -> int | None:
    """Resolve a CLI argument to a protocol number.

    Args:
        arg: A protocol number string (e.g. "924") or release tag (e.g. "r26_u0").

    Returns:
        The integer protocol number, or None if not found.
    """
    try:
        proto = int(arg)
        if proto in VERSIONS:
            return proto
    except ValueError:
        pass
    for ver in VERSIONS.values():
        if ver.release_tag == arg:
            return ver.protocol
    return None


def _load_packets(path: Path) -> list[PacketDefinition]:
    """Load packet definitions from a JSON file.

    Args:
        path: Path to the JSON file.

    Returns:
        List of PacketDefinition models.
    """
    with open(path, encoding="utf-8") as f:
        data = json.load(f)
    return [PacketDefinition.model_validate(entry) for entry in data]


def main() -> None:
    """Entry point: diff two protocol versions."""
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
        sys.exit(1)

    old_packets = _load_packets(old_path)
    new_packets = _load_packets(new_path)

    print(f"Comparing {len(old_packets)} old vs {len(new_packets)} new definitions...")
    diff = diff_packets(old_packets, new_packets, old_proto, new_proto)

    print(f"  New packets: {len(diff.new_packets)}")
    print(f"  New types: {len(diff.new_types)}")
    print(f"  Removed packets: {len(diff.removed_packets)}")
    print(f"  Removed types: {len(diff.removed_types)}")
    print(f"  Changed: {len(diff.changed_packets)}")

    out_path = DATA_DIR / f"v{old_proto}_v{new_proto}_diff.json"
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(diff.model_dump(exclude_defaults=True), f, indent=2)
    print(f"  Written to {out_path}")


if __name__ == "__main__":
    main()
