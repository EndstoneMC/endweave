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
import re
import sys
from pathlib import Path

from models import (
    ChangelogEntry,
    EnumChange,
    EnumEntry,
    PacketChanges,
    PacketDefinition,
    ProtocolDiff,
    TypeChange,
    is_node_id_noise,
)

PROJECT_ROOT = Path(__file__).resolve().parent.parent
DATA_DIR = PROJECT_ROOT / "data"
PROTOCOL_DOCS_DIR = PROJECT_ROOT / "protocol_docs"

sys.path.insert(0, str(PROJECT_ROOT / "src"))
from endstone_endweave.protocol.versions import VERSIONS  # noqa: E402

# ---------------------------------------------------------------------------
# Changelog parsing
# ---------------------------------------------------------------------------

_ADDED_RE = re.compile(r"Added\s+(.+?)\s+\(([^)]+)\)")
_DISPLACED_RE = re.compile(r"Displaced\s+(.+)")
_REMOVED_RE = re.compile(r"Removed\s+(.+)")
_CHANGED_RE = re.compile(r"Changed\s+(\S+)\s+from\s+.+\s+to\s+.+")
_RAW_ENTRY_RE = re.compile(r"^(\d+):\s+(.+)$")


def _clean_line(line: str) -> str:
    """Clean HTML entities and markdown escapes from a changelog line."""
    line = line.replace("&#x20;", " ")
    line = line.replace("\\_", "_")
    return line.strip()


def _parse_id(raw: str) -> int | None:
    """Try to parse a numeric ID from a changelog Added entry.

    Handles plain integers and hex expressions like "0x00010000 | Mob".
    Returns None for non-numeric or composite flag expressions.
    """
    raw = raw.strip()
    try:
        return int(raw)
    except ValueError:
        pass
    # Try hex-only value (no | composition)
    if "|" not in raw:
        try:
            return int(raw, 16)
        except ValueError:
            pass
    return None


def parse_changelog(
    path: Path,
) -> tuple[dict[str, EnumChange], list[ChangelogEntry]]:
    """Parse a protocol changelog markdown file.

    Args:
        path: Path to the changelog markdown file.

    Returns:
        Tuple of (enum_changes dict, raw changelog entries list).
    """
    if not path.exists():
        return {}, []

    text = path.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()

    enums: dict[str, EnumChange] = {}
    changelog: list[ChangelogEntry] = []
    current_enum: str | None = None
    in_raw_section = False

    for raw_line in lines:
        line = _clean_line(raw_line)
        if not line:
            continue

        # Detect section headers
        if line.startswith("\\##") or line.startswith("##"):
            header = line.lstrip("\\#").strip()
            in_raw_section = "Raw Protocol Version Changelog" in header
            current_enum = None
            continue

        if in_raw_section:
            m = _RAW_ENTRY_RE.match(line)
            if m:
                changelog.append(ChangelogEntry(
                    protocol=int(m.group(1)),
                    description=m.group(2).strip(),
                ))
            continue

        # Enum section: line ending with ":" that's not indented
        if line.endswith(":") and not line.startswith(" "):
            current_enum = line[:-1].strip()
            if current_enum not in enums:
                enums[current_enum] = EnumChange()
            continue

        if current_enum is None:
            continue

        enum = enums[current_enum]

        m = _ADDED_RE.search(line)
        if m:
            enum.added.append(EnumEntry(
                name=m.group(1).strip(),
                id=_parse_id(m.group(2)),
            ))
            continue

        m = _DISPLACED_RE.search(line)
        if m:
            enum.displaced.append(m.group(1).strip())
            continue

        m = _REMOVED_RE.search(line)
        if m:
            enum.removed.append(m.group(1).strip())
            continue

        m = _CHANGED_RE.search(line)
        if m:
            enum.changed.append(m.group(1).strip())
            continue

    return enums, changelog


def diff_changelogs(
    old_path: Path,
    new_path: Path,
    old_protocol: int,
) -> tuple[dict[str, EnumChange], list[ChangelogEntry]]:
    """Diff two changelogs to extract only changes new in the target version.

    Args:
        old_path: Path to the older version's changelog.
        new_path: Path to the newer version's changelog.
        old_protocol: Protocol number of the older version (for filtering
            raw changelog entries).

    Returns:
        Tuple of (new enum changes, new raw changelog entries).
    """
    old_enums, _ = parse_changelog(old_path)
    new_enums, new_changelog = parse_changelog(new_path)

    # Filter raw changelog to only entries after old_protocol
    filtered_changelog = [
        e for e in new_changelog if e.protocol > old_protocol
    ]

    # Diff enum sections: include entries in new but not in old
    diff_enums: dict[str, EnumChange] = {}
    for name, new_change in new_enums.items():
        if name not in old_enums:
            # Entire enum section is new
            diff_enums[name] = new_change
            continue

        old_change = old_enums[name]
        old_added_names = {e.name for e in old_change.added}
        old_displaced = set(old_change.displaced)
        old_removed = set(old_change.removed)
        old_changed = set(old_change.changed)

        new_added = [e for e in new_change.added if e.name not in old_added_names]
        new_displaced = [d for d in new_change.displaced if d not in old_displaced]
        new_removed = [r for r in new_change.removed if r not in old_removed]
        new_changed = [c for c in new_change.changed if c not in old_changed]

        # Detect renames: same ID, different name between old-only and new-only
        old_added_by_id: dict[int, str] = {
            e.id: e.name for e in old_change.added
            if e.id is not None and e.name not in {e2.name for e2 in new_change.added}
        }
        renamed_new_names: set[str] = set()
        for entry in new_added[:]:
            if entry.id is not None and entry.id in old_added_by_id:
                old_name = old_added_by_id[entry.id]
                new_changed.append(f"{old_name} -> {entry.name} ({entry.id})")
                renamed_new_names.add(entry.name)
        new_added = [e for e in new_added if e.name not in renamed_new_names]

        if new_added or new_displaced or new_removed or new_changed:
            diff_enums[name] = EnumChange(
                added=new_added,
                displaced=new_displaced,
                removed=new_removed,
                changed=new_changed,
            )

    return diff_enums, filtered_changelog


def _find_changelog(tag: str) -> Path | None:
    """Find the changelog file for a protocol version tag.

    Args:
        tag: Release tag (e.g. "r26_u0").

    Returns:
        Path to the changelog file, or None if not found.
    """
    docs_dir = PROTOCOL_DOCS_DIR / tag
    if not docs_dir.exists():
        return None
    changelogs = list(docs_dir.glob("changelog*.md"))
    return changelogs[0] if changelogs else None


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


def _filter_descendants(paths: list[str], extra_prefixes: list[str]) -> list[str]:
    """Remove paths that are children of other paths in the same list or of extra prefixes.

    Args:
        paths: Sorted list of dotted field paths.
        extra_prefixes: Additional prefixes to filter against (e.g. type_changes paths).

    Returns:
        Filtered list with only top-level entries.
    """
    result = []
    for path in paths:
        prefixes = [p + "." for p in result]
        if any(path.startswith(p) for p in prefixes):
            continue
        if any(path.startswith(p) for p in extra_prefixes):
            continue
        result.append(path)
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

    changed_packets: dict[str, PacketChanges] = {}
    changed_types: dict[str, PacketChanges] = {}

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
        added_set = {
            f for f in (new_field_names - old_field_names) if not is_node_id_noise(f)
        }
        removed_set = {
            f for f in (old_field_names - new_field_names) if not is_node_id_noise(f)
        }

        type_changes: dict[str, TypeChange] = {}
        for field_name in sorted(old_field_names & new_field_names):
            old_type = old_fields[field_name]["type"]
            new_type = new_fields[field_name]["type"]
            if old_type != new_type:
                type_changes[field_name] = TypeChange(old=old_type, new=new_type)

        # Promote add+remove pairs with shared parent to type_changes.
        # When a DOT field's leaf child changes name (e.g. "unsigned varint"
        # -> "varint"), flatten_fields produces different paths. Detect these
        # by matching parent paths and treat as type changes instead.
        added_by_parent: dict[str, list[str]] = {}
        for path in added_set:
            parent = path.rsplit(".", 1)[0] if "." in path else ""
            added_by_parent.setdefault(parent, []).append(path)
        removed_by_parent: dict[str, list[str]] = {}
        for path in removed_set:
            parent = path.rsplit(".", 1)[0] if "." in path else ""
            removed_by_parent.setdefault(parent, []).append(path)

        promoted: set[str] = set()
        for parent in set(added_by_parent) & set(removed_by_parent):
            a_paths = added_by_parent[parent]
            r_paths = removed_by_parent[parent]
            # Only promote when there's exactly one added and one removed
            # under the same parent -- a clear leaf type swap
            if len(a_paths) == 1 and len(r_paths) == 1:
                a_leaf = a_paths[0].rsplit(".", 1)[-1]
                r_leaf = r_paths[0].rsplit(".", 1)[-1]
                if parent and parent not in type_changes:
                    type_changes[parent] = TypeChange(old=r_leaf, new=a_leaf)
                promoted.add(a_paths[0])
                promoted.add(r_paths[0])

        added = sorted(added_set - promoted)
        removed = sorted(removed_set - promoted)

        # Filter out fields that are descendants of a type_changes path
        # or descendants of another field in the same list (leaf type
        # children like "loadFromJson.bool" under "loadFromJson").
        tc_prefixes = [p + "." for p in type_changes]
        added = _filter_descendants(added, tc_prefixes)
        removed = _filter_descendants(removed, tc_prefixes)

        if added or removed or type_changes:
            entry = PacketChanges(
                packet_id=old_def.packet_id,
                direction=old_def.direction,
                added_fields=added,
                removed_fields=removed,
                type_changes=type_changes,
            )
            if old_def.packet_id is not None:
                changed_packets[name] = entry
            else:
                changed_types[name] = entry

    # Deduplicate: when a changed_types entry's added/removed fields appear
    # as suffixed paths in other entries (parent types or packets that embed
    # the sub-type), remove the redundant fields from the parent and record
    # a changed_sub_types reference so handlers know where to look.
    subtype_suffix_to_name: dict[str, str] = {}
    for type_name, changes in changed_types.items():
        for f in changes.added_fields:
            subtype_suffix_to_name["." + f] = type_name
        for f in changes.removed_fields:
            subtype_suffix_to_name["." + f] = type_name

    if subtype_suffix_to_name:
        for entries in (changed_packets, changed_types):
            for changes in entries.values():
                # Find which sub-types are referenced and at what path prefix
                sub_type_refs: dict[str, str] = {}
                for field in changes.added_fields + changes.removed_fields:
                    for suffix, st_name in subtype_suffix_to_name.items():
                        if field.endswith(suffix):
                            prefix = field[: -len(suffix)]
                            if prefix:
                                sub_type_refs[prefix] = st_name

                changes.added_fields = [
                    f for f in changes.added_fields
                    if not any(f.endswith(s) for s in subtype_suffix_to_name)
                ]
                changes.removed_fields = [
                    f for f in changes.removed_fields
                    if not any(f.endswith(s) for s in subtype_suffix_to_name)
                ]

                # Merge sub-type references into type_changes
                for path, st_name in sub_type_refs.items():
                    if path not in changes.type_changes:
                        changes.type_changes[path] = TypeChange(
                            old=st_name, new=st_name
                        )

        # Remove changed_types entries that became empty after dedup.
        # Keep all changed_packets entries (even if empty) so LLMs know
        # which packets need handlers.
        changed_types = {
            k: v for k, v in changed_types.items()
            if v.added_fields or v.removed_fields or v.type_changes
        }

    return ProtocolDiff(
        old_protocol=old_protocol,
        new_protocol=new_protocol,
        new_packets=new_pkt_names,
        new_types=new_type_names,
        removed_packets=removed_pkt_names,
        removed_types=removed_type_names,
        changed_packets=changed_packets,
        changed_types=changed_types,
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

    old_path = DATA_DIR / f"v{old_proto}.json"
    new_path = DATA_DIR / f"v{new_proto}.json"

    if not old_path.exists() or not new_path.exists():
        print("Error: Run parse_protocol_docs.py first to generate packet JSON files.")
        print(f"  Expected: {old_path}")
        print(f"  Expected: {new_path}")
        sys.exit(1)

    old_packets = _load_packets(old_path)
    new_packets = _load_packets(new_path)

    print(f"Comparing {len(old_packets)} old vs {len(new_packets)} new definitions...")
    diff = diff_packets(old_packets, new_packets, old_proto, new_proto)

    # Parse and diff changelogs
    old_ver = VERSIONS[old_proto]
    new_ver = VERSIONS[new_proto]
    old_changelog_path = _find_changelog(old_ver.release_tag)
    new_changelog_path = _find_changelog(new_ver.release_tag)

    if old_changelog_path and new_changelog_path:
        enum_changes, changelog = diff_changelogs(
            old_changelog_path, new_changelog_path, old_proto
        )
        diff.enum_changes = dict(sorted(enum_changes.items()))
        diff.changelog = sorted(changelog, key=lambda e: e.protocol)
        print(f"  Enum changes: {len(diff.enum_changes)} enums")
        print(f"  Changelog entries: {len(diff.changelog)}")
    elif new_changelog_path:
        # No old changelog, use all entries from new
        enum_changes, changelog = parse_changelog(new_changelog_path)
        filtered = [e for e in changelog if e.protocol > old_proto]
        diff.enum_changes = dict(sorted(enum_changes.items()))
        diff.changelog = sorted(filtered, key=lambda e: e.protocol)
        print(f"  Enum changes: {len(diff.enum_changes)} enums")
        print(f"  Changelog entries: {len(diff.changelog)}")
    else:
        print("  No changelogs found")

    print(f"  New packets: {len(diff.new_packets)}")
    print(f"  New types: {len(diff.new_types)}")
    print(f"  Removed packets: {len(diff.removed_packets)}")
    print(f"  Removed types: {len(diff.removed_types)}")
    print(f"  Changed packets: {len(diff.changed_packets)}")
    print(f"  Changed types: {len(diff.changed_types)}")

    out_path = DATA_DIR / f"v{old_proto}_to_v{new_proto}.json"
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(diff.model_dump(exclude_defaults=True), f, indent=2)
    print(f"  Written to {out_path}")


if __name__ == "__main__":
    main()
