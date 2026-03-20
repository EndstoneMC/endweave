#!/usr/bin/env python3
"""Parse protocol documentation files (DOT + JSON) into structured JSON.

Reads .dot files from protocol_docs/<version>/dot/ and .json files from
protocol_docs/<version>/json/, then outputs merged structured JSON to
src/endstone_endweave/data/v<protocol>_packets.json.

Usage:
    python tools/parse_protocol_docs.py              # parse all known versions
    python tools/parse_protocol_docs.py r26_u0 r26_u1  # parse specific versions

DOT format:
- Node 0 = packet root, `comment` has metadata
- Child nodes = fields, `label` = name, `comment` has typeName + attributes
- attributes: 512 = leaf/primitive, 8 = list, 16 = list element example
- Edges define parent->child containment

JSON format:
- JSON Schema with x-underlying-type, x-serialization-options, x-ordinal-index
- $ref -> definitions for nested types
- $metaProperties["[cereal:packet]"] = packet ID
"""

import argparse
import json
import re
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent

sys.path.insert(0, str(PROJECT_ROOT / "src"))
from endstone_endweave.protocol.versions import VERSIONS  # noqa: E402

OUTPUT_DIR = PROJECT_ROOT / "data"

# ---------------------------------------------------------------------------
# DOT parsing (unchanged from parse_dot_files.py)
# ---------------------------------------------------------------------------

NODE_RE = re.compile(r"(\d+)\s*\[([^\]]*)\]", re.DOTALL)
ATTR_RE = re.compile(r'(\w+)\s*=\s*"([^"]*)"')
EDGE_RE = re.compile(r"(\d+)\s*->\s*(\d+)")


def parse_dot_file(filepath: Path) -> dict | None:
    """Parse a single .dot file into a structured dict.

    Args:
        filepath: Path to the .dot file.

    Returns:
        Dict with "name", "comment", "label", and "fields" keys describing
        the packet, or None if the file has no root node.
    """
    text = filepath.read_text(encoding="utf-8", errors="replace")

    nodes: dict[int, dict[str, str]] = {}
    edges: list[tuple[int, int]] = []

    for match in NODE_RE.finditer(text):
        node_id = int(match.group(1))
        attrs_str = match.group(2)
        attrs = {}
        for attr_match in ATTR_RE.finditer(attrs_str):
            attrs[attr_match.group(1)] = attr_match.group(2)
        nodes[node_id] = attrs

    for match in EDGE_RE.finditer(text):
        edges.append((int(match.group(1)), int(match.group(2))))

    if 0 not in nodes:
        return None

    children: dict[int, list[int]] = {}
    for parent, child in edges:
        children.setdefault(parent, []).append(child)

    root = nodes[0]

    def build_field(node_id: int) -> dict:
        node = nodes.get(node_id, {})
        comment = node.get("comment", "")

        type_name = ""
        attributes = 0
        for part in comment.split(","):
            part = part.strip()
            if part.startswith("typeName:"):
                type_name = part.split(":", 1)[1].strip()
            elif part.startswith("attributes:"):
                try:
                    attributes = int(part.split(":", 1)[1].strip())
                except ValueError:
                    pass

        field = {
            "name": node.get("label", f"node_{node_id}"),
            "type": type_name,
            "attributes": attributes,
        }

        child_ids = children.get(node_id, [])
        if child_ids:
            field["children"] = [build_field(cid) for cid in child_ids]

        return field

    root_children = children.get(0, [])
    fields = [build_field(cid) for cid in root_children]

    return {
        "name": filepath.stem,
        "comment": root.get("comment", ""),
        "label": root.get("label", filepath.stem),
        "fields": fields,
    }


def parse_dot_version(dot_dir: Path) -> list[dict]:
    """Parse all .dot files in a directory.

    Args:
        dot_dir: Path to the directory containing .dot files.

    Returns:
        List of structured packet dicts parsed from the .dot files.
    """
    if not dot_dir.exists():
        print(f"  Warning: {dot_dir} does not exist, skipping DOT")
        return []

    packets = []
    for dot_file in sorted(dot_dir.glob("*.dot")):
        result = parse_dot_file(dot_file)
        if result:
            packets.append(result)
    return packets


# ---------------------------------------------------------------------------
# JSON Schema parsing
# ---------------------------------------------------------------------------


def _resolve_type(prop: dict) -> str:
    """Map a JSON Schema property to a DOT-style type name.

    Args:
        prop: JSON Schema property dict (may contain x-underlying-type,
            x-serialization-options, and type).

    Returns:
        Resolved type name string (e.g. "varint32", "bool", "string").
    """
    underlying = prop.get("x-underlying-type", "")
    serialization = prop.get("x-serialization-options", [])
    has_compression = "Compression" in serialization
    schema_type = prop.get("type", "")

    if underlying:
        if has_compression:
            # Compressed integer types -> varint/varuint names
            mapping = {
                "int32": "varint32",
                "uint32": "varuint32",
                "int64": "varint64",
                "uint64": "varuint64",
            }
            if underlying in mapping:
                return mapping[underlying]
        # boolean -> bool
        if underlying == "boolean":
            return "bool"
        # Non-compressed: return raw type (uint8, float, float32, int32, etc.)
        return underlying

    if schema_type == "string":
        return "string"
    if schema_type in ("boolean", "bool"):
        return "bool"
    if schema_type == "number":
        return "float"

    return schema_type


def _build_fields_from_properties(
    properties: dict,
    definitions: dict,
) -> list[dict]:
    """Convert JSON Schema properties into the DOT-style field list.

    Args:
        properties: Dict of JSON Schema property names to their schemas.
        definitions: Top-level definitions dict for resolving $ref pointers.

    Returns:
        List of field dicts sorted by x-ordinal-index.
    """
    # Sort by x-ordinal-index
    sorted_props = sorted(
        properties.items(),
        key=lambda kv: kv[1].get("x-ordinal-index", 999),
    )

    fields = []
    for name, prop in sorted_props:
        field = _build_field(name, prop, definitions)
        fields.append(field)
    return fields


def _resolve_ref(prop: dict, definitions: dict) -> dict | None:
    """Follow a $ref to its definition, returning the definition dict.

    Args:
        prop: JSON Schema property dict that may contain a "$ref" key.
        definitions: Top-level definitions dict for resolving $ref pointers.

    Returns:
        The referenced definition dict, or None if no $ref is present or
        the target is not found.
    """
    ref = prop.get("$ref", "")
    if not ref:
        return None
    # Format: "#/definitions/1234567"
    def_id = ref.rsplit("/", 1)[-1]
    return definitions.get(def_id)


def _build_field(name: str, prop: dict, definitions: dict) -> dict:
    """Build a single field dict from a JSON Schema property.

    Args:
        name: Field name.
        prop: JSON Schema property dict for this field.
        definitions: Top-level definitions dict for resolving $ref pointers.

    Returns:
        Structured field dict with "name", "type", "attributes", and
        optional "children" keys.
    """
    ref_def = _resolve_ref(prop, definitions)

    if prop.get("type") == "array":
        # Array field
        items = prop.get("items", {})
        items_ref = _resolve_ref(items, definitions)

        if items_ref:
            # Array of a referenced type
            element_title = items_ref.get("title", "element")
            element_children = _build_fields_from_properties(
                items_ref.get("properties", {}), definitions
            )
            element_field = (
                {
                    "name": element_title,
                    "type": element_title,
                    "attributes": 16,  # array element
                    "children": element_children,
                }
                if element_children
                else {
                    "name": element_title,
                    "type": element_title,
                    "attributes": 16 | 512,
                }
            )
        else:
            # Array of inline/primitive items
            elem_type = _resolve_type(items)
            element_field = {
                "name": "element",
                "type": elem_type,
                "attributes": 16 | 512,
            }

        return {
            "name": name,
            "type": "",
            "attributes": 8,  # list
            "children": [element_field],
        }

    if ref_def:
        # Referenced composite type
        ref_title = ref_def.get("title", name)
        child_fields = _build_fields_from_properties(
            ref_def.get("properties", {}), definitions
        )
        if child_fields:
            return {
                "name": name,
                "type": ref_title,
                "attributes": 0,
                "children": child_fields,
            }
        return {
            "name": name,
            "type": ref_title,
            "attributes": 512,
        }

    if prop.get("enum") is not None:
        # Enum field - use the underlying type with compression info
        type_name = _resolve_type(prop)
        return {
            "name": name,
            "type": type_name,
            "attributes": 512,
        }

    # Primitive / leaf field
    type_name = _resolve_type(prop)
    return {
        "name": name,
        "type": type_name,
        "attributes": 512,
    }


def parse_json_file(filepath: Path) -> dict | None:
    """Parse a single JSON Schema protocol file into a structured dict.

    Args:
        filepath: Path to the JSON Schema file.

    Returns:
        Dict with "name", "comment", "label", and "fields" keys describing
        the packet, or None if the file is not a valid packet schema.
    """
    with open(filepath, encoding="utf-8") as f:
        data = json.load(f)

    # Skip non-dict files (e.g. __protocoldoc.json which is a list)
    if not isinstance(data, dict):
        return None

    title = data.get("title", "")
    if not title:
        return None

    meta = data.get("$metaProperties", {})
    packet_id = meta.get("[cereal:packet]")
    if packet_id is None:
        return None  # Not a packet

    definitions = data.get("definitions", {})
    properties = data.get("properties", {})

    fields = _build_fields_from_properties(properties, definitions)

    comment_parts = []
    if packet_id is not None:
        comment_parts.append(f"packetId:{packet_id}")
    description = data.get("description", "")
    if description:
        comment_parts.append(f"description:{description}")

    return {
        "name": title,
        "comment": ", ".join(comment_parts),
        "label": title,
        "fields": fields,
    }


def parse_json_version(json_dir: Path) -> list[dict]:
    """Parse all JSON Schema packet files in a directory.

    Args:
        json_dir: Path to the directory containing JSON Schema files.

    Returns:
        List of structured packet dicts parsed from the JSON files.
    """
    if not json_dir.exists():
        print(f"  Warning: {json_dir} does not exist, skipping JSON")
        return []

    packets = []
    for json_file in sorted(json_dir.glob("*.json")):
        result = parse_json_file(json_file)
        if result:
            packets.append(result)
    return packets


# ---------------------------------------------------------------------------
# Merge and main
# ---------------------------------------------------------------------------


def merge_packets(dot_packets: list[dict], json_packets: list[dict]) -> list[dict]:
    """Merge DOT and JSON packets, preferring JSON for overlaps.

    Args:
        dot_packets: Packet dicts parsed from .dot files.
        json_packets: Packet dicts parsed from JSON Schema files.

    Returns:
        Sorted list of merged packet dicts, deduplicated by name.
    """
    by_name: dict[str, dict] = {}

    # Add DOT packets first
    for pkt in dot_packets:
        by_name[pkt["name"]] = pkt

    # JSON packets overwrite any DOT overlap
    for pkt in json_packets:
        by_name[pkt["name"]] = pkt

    return sorted(by_name.values(), key=lambda p: p["name"])


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Parse protocol DOT + JSON docs into structured JSON."
    )
    parser.add_argument(
        "versions",
        nargs="*",
        help="Release tags to parse (e.g. r26_u0 r26_u1). Defaults to all known versions.",
    )
    args = parser.parse_args()

    # Build config from versions registry
    all_configs: dict[str, dict] = {}
    for ver in VERSIONS.values():
        tag = ver.release_tag
        all_configs[tag] = {
            "dot_dir": f"protocol_docs/{tag}/dot",
            "json_dir": f"protocol_docs/{tag}/json",
            "output": f"v{ver.protocol}_packets.json",
        }

    if args.versions:
        selected = {}
        for tag in args.versions:
            if tag not in all_configs:
                print(
                    f"Error: unknown version tag '{tag}'. Known: {', '.join(all_configs)}"
                )
                sys.exit(1)
            selected[tag] = all_configs[tag]
    else:
        selected = all_configs

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    for version_name, cfg in selected.items():
        dot_dir = PROJECT_ROOT / cfg["dot_dir"]
        json_dir = PROJECT_ROOT / cfg["json_dir"]

        print(f"Parsing {version_name}...")

        dot_packets = parse_dot_version(dot_dir)
        print(f"  DOT: {len(dot_packets)} packets")

        json_packets = parse_json_version(json_dir)
        print(f"  JSON: {len(json_packets)} packets")

        merged = merge_packets(dot_packets, json_packets)
        print(f"  Merged: {len(merged)} packets")

        out_path = OUTPUT_DIR / cfg["output"]
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(merged, f, indent=2)
        print(f"  Written to {out_path}")

    print("Done.")


if __name__ == "__main__":
    main()
