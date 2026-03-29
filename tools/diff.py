#!/usr/bin/env python3
"""Generate a diff between two protocol version JSON files.

Automatically fetches and parses protocol docs if needed.

Usage:
    uv run tools/diff.py                  # diff lowest vs highest known
    uv run tools/diff.py r26_u0 r26_u1    # diff specific versions by tag
    uv run tools/diff.py 924 944           # diff by protocol number
"""

import argparse
import json
import re
import sys
from pathlib import Path

from fetch import fetch_if_missing
from models import (
    ChangelogEntry,
    EnumChange,
    EnumEntry,
    Field,
    PacketChanges,
    PacketDefinition,
    ProtocolDiff,
    TypeChange,
    is_node_id_noise,
)
from parse import ensure_parsed

PROJECT_ROOT = Path(__file__).resolve().parent.parent
DATA_DIR = PROJECT_ROOT / "data"
PROTOCOL_DOCS_DIR = PROJECT_ROOT / "protocol_docs"

sys.path.insert(0, str(PROJECT_ROOT / "src"))
from endstone_endweave.protocol.versions import VERSIONS  # noqa: E402

# ---------------------------------------------------------------------------
# Manual packet-to-subtype mappings
# ---------------------------------------------------------------------------
# The DOT/JSON schemas sometimes use different type names for structures
# that are semantically the same. These mappings connect packets to their
# embedded sub-types when the schema can't express the relationship.
#
# Format: "PacketName" -> {"field.path": "SubTypeName"}
#
# When a sub-type listed here appears in changed_types, the packet will
# be added to changed_packets with a type_changes reference.
PACKET_SUBTYPE_OVERRIDES: dict[str, dict[str, str]] = {
    # InventoryTransactionPacket embeds PackedItemUseLegacyInventoryTransaction
    # for UseItem transactions, but DOT uses "InventoryTransaction" type instead
    "InventoryTransactionPacket": {
        "Transaction.ItemUse": "PackedItemUseLegacyInventoryTransaction",
    },
    # CameraSplinePacket uses CameraSplineDefinition in JSON/DOT, but the
    # spline instruction fields are modeled as CameraInstruction::SplineInstruction
    "CameraSplinePacket": {
        "Splines.SplineInstruction": "CameraInstruction::SplineInstruction",
    },
}

# ---------------------------------------------------------------------------
# Changelog parsing
# ---------------------------------------------------------------------------

_ADDED_RE = re.compile(r"Added\s+(.+?)\s+\(([^)]+)\)")
_DISPLACED_RE = re.compile(r"Displaced\s+(.+)")
_REMOVED_RE = re.compile(r"Removed\s+(.+)")
_CHANGED_RE = re.compile(r"Changed\s+(\S+)\s+from\s+.+\s+to\s+.+")
_RAW_ENTRY_RE = re.compile(r"^(\d+):\s+(.+)$")
_RENAME_RE = re.compile(r"^(.+?)\s*->\s*(.+?)\s*\(\d+\)$")


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


def _find_changelog(protocol: int) -> Path | None:
    """Find the changelog file for a protocol version."""
    docs_dir = PROTOCOL_DOCS_DIR / f"v{protocol}"
    if not docs_dir.exists():
        return None
    changelogs = list(docs_dir.glob("changelog*.md"))
    return changelogs[0] if changelogs else None


# ---------------------------------------------------------------------------
# Packet diffing
# ---------------------------------------------------------------------------


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


def _find_type_refs(
    fields: list[Field], type_names: set[str], prefix: str = "",
) -> dict[str, str]:
    """Find fields whose type matches a known changed type name.

    Args:
        fields: Field tree to search.
        type_names: Set of changed type names to look for.
        prefix: Dotted path prefix for recursion.

    Returns:
        Dict mapping dotted field path to the matched type name.
    """
    refs: dict[str, str] = {}
    for field in fields:
        path = f"{prefix}.{field.name}" if prefix else field.name
        if field.type in type_names:
            refs[path] = field.type
        if field.children:
            refs.update(_find_type_refs(field.children, type_names, path))
    return refs


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


def _group_by_parent(paths: set[str]) -> dict[str, list[str]]:
    """Group dotted field paths by their parent path."""
    groups: dict[str, list[str]] = {}
    for path in paths:
        parent = path.rsplit(".", 1)[0] if "." in path else ""
        groups.setdefault(parent, []).append(path)
    return groups


def _promote_leaf_renames(
    added: set[str],
    removed: set[str],
    type_changes: dict[str, TypeChange],
) -> set[str]:
    """Detect add+remove pairs under the same parent and promote to type changes.

    When a DOT field's leaf child changes name, flatten_fields produces
    different paths. Matching single-child add/remove pairs under the same
    parent catches these and records them as type changes instead.

    Args:
        added: Set of added field paths.
        removed: Set of removed field paths.
        type_changes: Existing type changes dict (mutated: promotions added).

    Returns:
        Set of paths that were promoted (to exclude from added/removed).
    """
    added_by_parent = _group_by_parent(added)
    removed_by_parent = _group_by_parent(removed)

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
    return promoted


def _diff_definition(
    old_def: PacketDefinition,
    new_def: PacketDefinition,
) -> PacketChanges | None:
    """Compute field-level diff between two versions of the same definition.

    Args:
        old_def: Definition from the older protocol version.
        new_def: Definition from the newer protocol version.

    Returns:
        PacketChanges if there are differences, None otherwise.
    """
    old_fields = flatten_fields(
        [f.model_dump(exclude_defaults=True) for f in old_def.fields]
    )
    new_fields = flatten_fields(
        [f.model_dump(exclude_defaults=True) for f in new_def.fields]
    )

    old_paths = set(old_fields)
    new_paths = set(new_fields)

    # Filter out DOT node ID noise
    added_set = {f for f in (new_paths - old_paths) if not is_node_id_noise(f)}
    removed_set = {f for f in (old_paths - new_paths) if not is_node_id_noise(f)}

    # Skip mPayload wrapper noise: between some versions, Mojang wrapped
    # all packet fields in an "mPayload" container. When the only removed
    # fields are mPayload or mPayload.*, this is a schema change, not a
    # real protocol change.
    if removed_set and all(
        f == "mPayload" or f.startswith("mPayload.") for f in removed_set
    ):
        has_real_changes = any(
            old_fields[f]["type"] != new_fields[f]["type"]
            for f in old_paths & new_paths
            if not f.startswith("mPayload")
        )
        if not has_real_changes:
            return None

    # Find type changes on shared fields
    type_changes: dict[str, TypeChange] = {}
    for field_name in sorted(old_paths & new_paths):
        old_type = old_fields[field_name]["type"]
        new_type = new_fields[field_name]["type"]
        if old_type != new_type:
            type_changes[field_name] = TypeChange(old=old_type, new=new_type)

    # Promote add+remove leaf pairs to type changes
    promoted = _promote_leaf_renames(added_set, removed_set, type_changes)
    added = sorted(added_set - promoted)
    removed = sorted(removed_set - promoted)

    # Filter out descendants of type_changes paths or other entries
    tc_prefixes = [p + "." for p in type_changes]
    added = _filter_descendants(added, tc_prefixes)
    removed = _filter_descendants(removed, tc_prefixes)

    if not added and not removed and not type_changes:
        return None

    return PacketChanges(
        packet_id=old_def.packet_id,
        direction=old_def.direction,
        added_fields=added,
        removed_fields=removed,
        type_changes=type_changes,
    )


def _dedup_subtypes(
    changed_packets: dict[str, PacketChanges],
    changed_types: dict[str, PacketChanges],
) -> None:
    """Remove redundant subtype fields from parent entries.

    When a changed_types entry's added/removed fields appear as suffixed
    paths in other entries, remove the redundant fields and add a
    type_changes reference instead.  Empty changed_types entries are pruned.

    Mutates both dicts in place.
    """
    subtype_suffix_to_name: dict[str, str] = {}
    for type_name, changes in changed_types.items():
        for f in changes.added_fields:
            subtype_suffix_to_name["." + f] = type_name
        for f in changes.removed_fields:
            subtype_suffix_to_name["." + f] = type_name

    if not subtype_suffix_to_name:
        return

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
    for k in [k for k, v in changed_types.items()
              if not v.added_fields and not v.removed_fields and not v.type_changes]:
        del changed_types[k]


def _changes_from_type_refs(
    pkt_def: PacketDefinition,
    refs: dict[str, str],
) -> PacketChanges:
    """Create a PacketChanges entry from type reference matches."""
    tc = {path: TypeChange(old=name, new=name) for path, name in refs.items()}
    return PacketChanges(
        packet_id=pkt_def.packet_id,
        direction=pkt_def.direction,
        type_changes=tc,
    )


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

    old_names = set(old_by_name)
    new_names = set(new_by_name)

    added_names = sorted(new_names - old_names)
    removed_names = sorted(old_names - new_names)

    # Separate packets (have packet_id) from sub-types
    new_pkt_names = [n for n in added_names if new_by_name[n].packet_id is not None]
    new_type_names = [n for n in added_names if new_by_name[n].packet_id is None]
    removed_pkt_names = [n for n in removed_names if old_by_name[n].packet_id is not None]
    removed_type_names = [n for n in removed_names if old_by_name[n].packet_id is None]

    # Per-definition field diffs
    changed_packets: dict[str, PacketChanges] = {}
    changed_types: dict[str, PacketChanges] = {}

    for name in sorted(old_names & new_names):
        entry = _diff_definition(old_by_name[name], new_by_name[name])
        if entry is None:
            continue
        if old_by_name[name].packet_id is not None:
            changed_packets[name] = entry
        else:
            changed_types[name] = entry

    # Remove redundant subtype fields from parent entries
    _dedup_subtypes(changed_packets, changed_types)

    # Scan all packet definitions for fields that embed a changed sub-type.
    # Packets that embed changed types need handlers even if they have no
    # direct field changes themselves.
    changed_type_names = set(changed_types)
    if changed_type_names:
        all_pkt_defs = {
            p.name: p
            for p in list(old_packets) + list(new_packets)
            if p.packet_id is not None
        }

        for pkt_name, pkt_def in all_pkt_defs.items():
            if pkt_name in changed_packets:
                continue
            refs = _find_type_refs(pkt_def.fields, changed_type_names)
            if refs:
                changed_packets[pkt_name] = _changes_from_type_refs(pkt_def, refs)

        # "New" packets that embed changed types are actually translations,
        # not truly new -- reclassify them as changed_packets.
        still_new = []
        for pkt_name in new_pkt_names:
            pkt_def = new_by_name.get(pkt_name)
            if pkt_def is None:
                still_new.append(pkt_name)
                continue
            refs = _find_type_refs(pkt_def.fields, changed_type_names)
            if refs and pkt_name not in changed_packets:
                changed_packets[pkt_name] = _changes_from_type_refs(pkt_def, refs)
            elif not refs:
                still_new.append(pkt_name)
        new_pkt_names = still_new

        # Apply manual packet-to-subtype overrides for relationships that
        # DOT/JSON schemas don't capture.
        for pkt_name, sub_map in PACKET_SUBTYPE_OVERRIDES.items():
            if pkt_name in changed_packets:
                continue
            pkt_def = all_pkt_defs.get(pkt_name)
            if pkt_def is None:
                continue
            refs = {
                path: sub_name
                for path, sub_name in sub_map.items()
                if sub_name in changed_types
            }
            if refs:
                changed_packets[pkt_name] = _changes_from_type_refs(pkt_def, refs)

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


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


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
    """Load packet definitions from a JSON file."""
    with open(path, encoding="utf-8") as f:
        data = json.load(f)
    return [PacketDefinition.model_validate(entry) for entry in data]


def _process_changelogs(
    diff: ProtocolDiff, old_proto: int, new_proto: int,
) -> None:
    """Parse changelogs and merge enum/changelog data into the diff."""
    old_changelog = _find_changelog(old_proto)
    new_changelog = _find_changelog(new_proto)

    if not new_changelog:
        print("  No changelogs found")
        return

    if old_changelog:
        enum_changes, changelog = diff_changelogs(
            old_changelog, new_changelog, old_proto
        )
    else:
        enum_changes, changelog = parse_changelog(new_changelog)
        changelog = [e for e in changelog if e.protocol > old_proto]

    diff.enum_changes = dict(sorted(enum_changes.items()))
    diff.changelog = sorted(changelog, key=lambda e: e.protocol)
    print(f"  Enum changes: {len(diff.enum_changes)} enums")
    print(f"  Changelog entries: {len(diff.changelog)}")


def _extract_renames(diff: ProtocolDiff) -> None:
    """Move renamed packets from new/removed lists to renamed_packets.

    Detects renames from MinecraftPacketIds enum "Changed" entries
    with the pattern "OldName -> NewName (ID)".
    """
    pkt_id_changes = diff.enum_changes.get("MinecraftPacketIds")
    if not pkt_id_changes:
        return
    for entry in pkt_id_changes.changed:
        m = _RENAME_RE.match(entry)
        if m:
            old_name = m.group(1).strip() + "Packet"
            new_name = m.group(2).strip() + "Packet"
            if old_name in diff.removed_packets:
                diff.removed_packets.remove(old_name)
            if new_name in diff.new_packets:
                diff.new_packets.remove(new_name)
            diff.renamed_packets[old_name] = new_name


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

    # Auto-fetch and parse if needed
    for proto in (old_proto, new_proto):
        fetch_if_missing(proto)
        ensure_parsed(proto)

    old_packets = _load_packets(DATA_DIR / f"v{old_proto}.json")
    new_packets = _load_packets(DATA_DIR / f"v{new_proto}.json")

    print(f"Comparing {len(old_packets)} old vs {len(new_packets)} new definitions...")
    diff = diff_packets(old_packets, new_packets, old_proto, new_proto)

    _process_changelogs(diff, old_proto, new_proto)
    _extract_renames(diff)

    print(f"  New packets: {len(diff.new_packets)}")
    print(f"  New types: {len(diff.new_types)}")
    print(f"  Removed packets: {len(diff.removed_packets)}")
    print(f"  Removed types: {len(diff.removed_types)}")
    print(f"  Renamed packets: {len(diff.renamed_packets)}")
    print(f"  Changed packets: {len(diff.changed_packets)}")
    print(f"  Changed types: {len(diff.changed_types)}")

    out_path = DATA_DIR / f"v{old_proto}_to_v{new_proto}.json"
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(diff.model_dump(exclude_defaults=True), f, indent=2)
    print(f"  Written to {out_path}")


if __name__ == "__main__":
    main()
