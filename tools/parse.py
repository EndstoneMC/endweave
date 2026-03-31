#!/usr/bin/env python3
"""Parse protocol documentation files (DOT + JSON) into structured JSON.

Reads from protocol_docs/v<protocol>/{dot,json}/ and outputs to
data/v<protocol>.json.

Usage:
    uv run tools/parse.py              # parse all known versions
    uv run tools/parse.py 924 944      # parse specific versions
"""

import argparse
import json
import re
import sys
from pathlib import Path

import pydot
from models import Field, PacketDefinition, infer_direction

PROJECT_ROOT = Path(__file__).resolve().parent.parent

sys.path.insert(0, str(PROJECT_ROOT / "src"))
from endstone_endweave.protocol.versions import VERSIONS  # noqa: E402

OUTPUT_DIR = PROJECT_ROOT / "data"

# ---------------------------------------------------------------------------
# DOT comment parsing
# ---------------------------------------------------------------------------

_COMMENT_KV_RE = re.compile(r'(\w+):\s*(?:"([^"]*)"|(-?\d+))')


def _parse_comment(raw: str) -> dict[str, str | int]:
    """Parse a DOT node comment string into key-value pairs.

    The comment format is:
        name: "Foo", typeName: "Bar", id: 42, branchId: 0, ...

    Args:
        raw: Raw comment string from pydot (may have outer quotes/escapes).

    Returns:
        Dict mapping keys to string or int values.
    """
    # Strip outer quotes added by pydot
    s = raw.strip('"')
    # Unescape inner quotes
    s = s.replace('\\"', '"')
    result: dict[str, str | int] = {}
    for m in _COMMENT_KV_RE.finditer(s):
        key = m.group(1)
        if m.group(2) is not None:
            result[key] = m.group(2)
        else:
            result[key] = int(m.group(3))
    return result


def _strip_quotes(s: str | None) -> str:
    """Strip surrounding double quotes from a pydot attribute value."""
    if s is None:
        return ""
    return s.strip('"')


# ---------------------------------------------------------------------------
# DOT parsing with pydot
# ---------------------------------------------------------------------------


def _lowest_numeric(ids: set[str]) -> str | None:
    """Pick the ID with the lowest numeric value from a set of node IDs."""
    numeric = []
    for nid in ids:
        try:
            numeric.append((int(nid), nid))
        except ValueError:
            pass
    if numeric:
        numeric.sort()
        return numeric[0][1]
    return None


def _parse_dot_graph(graph: pydot.Dot) -> tuple[str, dict[str, dict[str, str | int]], dict[str, list[str]]]:
    """Extract nodes and edges from a pydot graph.

    Args:
        graph: Parsed pydot graph.

    Returns:
        Tuple of (root_id, nodes_by_id, children_by_id) where:
        - root_id: The ID of the root node
        - nodes_by_id: Dict mapping node ID to parsed comment dict
        - children_by_id: Dict mapping node ID to list of child node IDs
    """
    # Collect nodes with attributes (skip bare declarations)
    nodes_by_id: dict[str, dict[str, str | int]] = {}
    for node in graph.get_nodes():
        comment = node.get_comment()
        if comment is None:
            continue
        node_id = node.get_name()
        parsed = _parse_comment(comment)
        # Store label as fallback
        label = _strip_quotes(node.get_label())
        if label:
            parsed.setdefault("name", label)
        nodes_by_id[node_id] = parsed

    # Build edge map
    children_by_id: dict[str, list[str]] = {}
    sources: set[str] = set()
    destinations: set[str] = set()
    for edge in graph.get_edges():
        src = edge.get_source()
        dst = edge.get_destination()
        sources.add(src)
        destinations.add(dst)
        children_by_id.setdefault(src, []).append(dst)

    # Find root: node with outgoing edges but no incoming edges
    root_candidates = sources - destinations
    if not root_candidates:
        root_id = _lowest_numeric(set(nodes_by_id)) or next(iter(nodes_by_id), "")
    elif len(root_candidates) == 1:
        root_id = root_candidates.pop()
    else:
        root_id = _lowest_numeric(root_candidates) or sorted(root_candidates)[0]

    return root_id, nodes_by_id, children_by_id


def _build_field_tree(
    node_id: str,
    nodes: dict[str, dict[str, str | int]],
    children: dict[str, list[str]],
) -> Field:
    """Recursively build a Field tree from DOT node data.

    Args:
        node_id: Current node ID.
        nodes: All parsed node data.
        children: Edge map (parent -> children).

    Returns:
        Field representing this node and its descendants.
    """
    node = nodes.get(node_id, {})
    name = str(node.get("name", f"node_{node_id}"))
    type_name = str(node.get("typeName", ""))
    attributes = int(node.get("attributes", 0))

    child_ids = children.get(node_id, [])
    child_fields = [_build_field_tree(cid, nodes, children) for cid in child_ids]

    return Field(
        name=name,
        type=type_name,
        attributes=attributes,
        fields=child_fields,
    )


def parse_dot_file(filepath: Path) -> PacketDefinition | None:
    """Parse a single DOT file into a PacketDefinition.

    Args:
        filepath: Path to the .dot file.

    Returns:
        PacketDefinition, or None if the file cannot be parsed.
    """
    try:
        graphs = pydot.graph_from_dot_file(str(filepath))
    except Exception:
        return None

    if not graphs:
        return None
    graph = graphs[0]

    graph_name = _strip_quotes(graph.get_name())
    if not graph_name:
        return None

    root_id, nodes, edge_children = _parse_dot_graph(graph)
    if root_id not in nodes:
        return None

    root_data = nodes[root_id]
    root_name = str(root_data.get("name", graph_name))

    # Extract packet_id from branchId on root node
    branch_id = root_data.get("branchId")
    packet_id: int | None = None
    if isinstance(branch_id, int) and branch_id > 0:
        packet_id = branch_id

    # Build field tree from root's children
    child_ids = edge_children.get(root_id, [])
    fields = [_build_field_tree(cid, nodes, edge_children) for cid in child_ids]

    return PacketDefinition(
        name=root_name,
        packet_id=packet_id,
        direction=infer_direction(root_name),
        fields=fields,
    )


def parse_dot_dir(dot_dir: Path) -> list[PacketDefinition]:
    """Parse all DOT files in a directory.

    Args:
        dot_dir: Path to the directory containing .dot files.

    Returns:
        List of parsed PacketDefinitions (includes both packets and sub-types).
    """
    if not dot_dir.exists():
        print(f"  Warning: {dot_dir} does not exist, skipping DOT")
        return []

    results = []
    for dot_file in sorted(dot_dir.glob("*.dot")):
        result = parse_dot_file(dot_file)
        if result:
            results.append(result)
    return results


# ---------------------------------------------------------------------------
# Sub-type resolution
# ---------------------------------------------------------------------------


def _resolve_field(
    field: Field,
    type_registry: dict[str, list[Field]],
    visited: set[str],
) -> Field:
    """Recursively resolve opaque type references using the type registry.

    When a field references a type (attributes & 256 on parent, child is
    attributes & 512 leaf with matching typeName), replace the opaque leaf
    with the resolved sub-type fields.

    Args:
        field: Field to resolve.
        type_registry: Map of type names to their field lists.
        visited: Set of type names currently being resolved (cycle detection).

    Returns:
        New Field with resolved children.
    """
    if not field.fields:
        return field

    new_children = []
    for child in field.fields:
        # Check if this child is an opaque reference that can be resolved
        # Pattern: parent has attributes & 256 (reference), child is leaf (512)
        # and child's name matches a known type
        if (
            (field.attributes & 256)
            and (child.attributes & 512)
            and not child.fields
            and child.name in type_registry
            and child.name not in visited
        ):
            # Replace opaque leaf with resolved sub-type fields
            resolved_fields = type_registry[child.name]
            resolved_children = []
            for rf in resolved_fields:
                resolved_children.append(_resolve_field(rf, type_registry, visited | {child.name}))
            new_children.append(
                Field(
                    name=child.name,
                    type=child.type,
                    attributes=child.attributes & ~512,  # No longer a leaf
                    fields=resolved_children,
                )
            )
        else:
            # Recurse into children
            new_children.append(_resolve_field(child, type_registry, visited))

    return Field(
        name=field.name,
        type=field.type,
        attributes=field.attributes,
        fields=new_children,
    )


def resolve_types(
    definitions: list[PacketDefinition],
    type_registry: dict[str, list[Field]],
) -> list[PacketDefinition]:
    """Resolve opaque type references in all packet definitions.

    Args:
        definitions: List of parsed packet/type definitions.
        type_registry: Map of type names to their resolved field lists.

    Returns:
        New list with resolved type references.
    """
    resolved = []
    for defn in definitions:
        new_fields = []
        for field in defn.fields:
            new_fields.append(_resolve_field(field, type_registry, set()))
        resolved.append(defn.model_copy(update={"fields": new_fields}))
    return resolved


def build_type_registry(definitions: list[PacketDefinition]) -> dict[str, list[Field]]:
    """Build a type registry from all parsed definitions.

    Args:
        definitions: All parsed packet and type definitions.

    Returns:
        Dict mapping type names to their top-level field lists.
    """
    registry: dict[str, list[Field]] = {}
    for defn in definitions:
        if defn.fields:
            registry[defn.name] = defn.fields
    return registry


# ---------------------------------------------------------------------------
# JSON Schema parsing
# ---------------------------------------------------------------------------


def _resolve_type(prop: dict) -> str:
    """Map a JSON Schema property to a type name.

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
            mapping = {
                "int32": "varint32",
                "uint32": "varuint32",
                "int64": "varint64",
                "uint64": "varuint64",
            }
            if underlying in mapping:
                return mapping[underlying]
        if underlying == "boolean":
            return "bool"
        return underlying

    if schema_type == "string":
        return "string"
    if schema_type in ("boolean", "bool"):
        return "bool"
    if schema_type == "number":
        return "float"

    return schema_type


def _props_to_fields(
    properties: dict,
    definitions: dict,
    required: list[str] | None = None,
) -> list[Field]:
    """Convert JSON Schema properties into Field list.

    Args:
        properties: Dict of JSON Schema property names to their schemas.
        definitions: Top-level definitions dict for resolving $ref pointers.
        required: List of required property names. Fields not in this list
            are marked as optional.

    Returns:
        List of Fields sorted by x-ordinal-index.
    """
    required_set = set(required) if required else None
    sorted_props = sorted(
        properties.items(),
        key=lambda kv: kv[1].get("x-ordinal-index", 999),
    )
    fields = [_build_json_field(name, prop, definitions) for name, prop in sorted_props]
    if required_set is not None:
        for field in fields:
            if field.name not in required_set:
                field.optional = True
    return fields


def _resolve_ref(prop: dict, definitions: dict) -> dict | None:
    """Follow a $ref to its definition.

    Args:
        prop: JSON Schema property dict that may contain a "$ref" key.
        definitions: Top-level definitions dict for resolving $ref pointers.

    Returns:
        The referenced definition dict, or None if not found.
    """
    ref = prop.get("$ref", "")
    if not ref:
        return None
    def_id = ref.rsplit("/", 1)[-1]
    return definitions.get(def_id)


def _build_json_field(name: str, prop: dict, definitions: dict) -> Field:
    """Build a single Field from a JSON Schema property.

    Args:
        name: Field name.
        prop: JSON Schema property dict.
        definitions: Top-level definitions dict for resolving $ref pointers.

    Returns:
        Structured Field.
    """
    ref_def = _resolve_ref(prop, definitions)

    if prop.get("type") == "array":
        items = prop.get("items", {})
        items_ref = _resolve_ref(items, definitions)

        if items_ref:
            element_title = items_ref.get("title", "element")
            element_children = _props_to_fields(items_ref.get("properties", {}), definitions, items_ref.get("required"))
            element_field = Field(
                name=element_title,
                type=element_title,
                attributes=16 if element_children else (16 | 512),
                fields=element_children,
            )
        else:
            elem_type = _resolve_type(items)
            element_field = Field(
                name="element",
                type=elem_type,
                attributes=16 | 512,
            )

        return Field(name=name, type="", attributes=8, fields=[element_field])

    if ref_def:
        ref_title = ref_def.get("title", name)
        child_fields = _props_to_fields(ref_def.get("properties", {}), definitions, ref_def.get("required"))
        if child_fields:
            return Field(name=name, type=ref_title, attributes=0, fields=child_fields)
        return Field(name=name, type=ref_title, attributes=512)

    return Field(name=name, type=_resolve_type(prop), attributes=512)


def parse_json_file(filepath: Path) -> PacketDefinition | None:
    """Parse a single JSON Schema protocol file.

    Args:
        filepath: Path to the JSON Schema file.

    Returns:
        PacketDefinition, or None if the file is not a valid packet schema.
    """
    with open(filepath, encoding="utf-8") as f:
        data = json.load(f)

    if not isinstance(data, dict):
        return None

    title = data.get("title", "")
    if not title:
        return None

    meta = data.get("$metaProperties", {})
    packet_id = meta.get("[cereal:packet]")
    if packet_id is None:
        return None

    definitions = data.get("definitions", {})
    properties = data.get("properties", {})
    required = data.get("required")
    fields = _props_to_fields(properties, definitions, required)
    description = data.get("description", "")

    return PacketDefinition(
        name=title,
        packet_id=int(packet_id) if packet_id is not None else None,
        direction=infer_direction(title),
        description=description,
        fields=fields,
    )


def parse_json_dir(json_dir: Path) -> list[PacketDefinition]:
    """Parse all JSON Schema packet files in a directory.

    Args:
        json_dir: Path to the directory containing JSON Schema files.

    Returns:
        List of parsed PacketDefinitions.
    """
    if not json_dir.exists():
        print(f"  Warning: {json_dir} does not exist, skipping JSON")
        return []

    results = []
    for json_file in sorted(json_dir.glob("*.json")):
        result = parse_json_file(json_file)
        if result:
            results.append(result)
    return results


# ---------------------------------------------------------------------------
# Output conversion
# ---------------------------------------------------------------------------


def _to_output(field: Field) -> dict:
    """Convert an internal Field tree to the clean output format.

    Flattens list element nesting: a list field's single element child
    is folded into the parent, with element type becoming the list's type
    and element children becoming the list's fields.
    """
    result: dict = {"name": field.name}

    if field.attributes & 8:  # List container
        result["list"] = True
        if field.fields:
            element = field.fields[0]
            if element.attributes & 16:  # Element child
                if element.type:
                    result["type"] = element.type
                if element.fields:
                    result["fields"] = [_to_output(c) for c in element.fields]
        return result

    if field.type:
        result["type"] = field.type
    if field.optional:
        result["optional"] = True
    if field.fields:
        result["fields"] = [_to_output(c) for c in field.fields]
    return result


# ---------------------------------------------------------------------------
# Merge and main
# ---------------------------------------------------------------------------


def merge_packets(
    dot_packets: list[PacketDefinition],
    json_packets: list[PacketDefinition],
) -> list[PacketDefinition]:
    """Merge DOT and JSON packets, preferring JSON for overlaps.

    For overlapping entries, JSON packet_id and direction are preserved.
    DOT-only entries keep their DOT-derived metadata.

    Args:
        dot_packets: Definitions parsed from .dot files.
        json_packets: Definitions parsed from JSON Schema files.

    Returns:
        Sorted list of merged definitions, deduplicated by name.
    """
    by_name: dict[str, PacketDefinition] = {}

    for pkt in dot_packets:
        by_name[pkt.name] = pkt

    for pkt in json_packets:
        by_name[pkt.name] = pkt

    return sorted(by_name.values(), key=lambda p: p.name)


PROTOCOL_DOCS_DIR = PROJECT_ROOT / "protocol_docs"


def parse_version(protocol: int) -> None:
    """Parse protocol docs for a single version into data/v{N}.json."""
    dot_dir = PROTOCOL_DOCS_DIR / f"v{protocol}" / "dot"
    json_dir = PROTOCOL_DOCS_DIR / f"v{protocol}" / "json"

    print(f"Parsing v{protocol}...")

    dot_packets = parse_dot_dir(dot_dir)
    print(f"  DOT: {len(dot_packets)} definitions")

    type_registry = build_type_registry(dot_packets)
    dot_packets = resolve_types(dot_packets, type_registry)
    print(f"  DOT: resolved sub-type references ({len(type_registry)} types in registry)")

    json_packets = parse_json_dir(json_dir)
    print(f"  JSON: {len(json_packets)} packets")

    merged = merge_packets(dot_packets, json_packets)
    print(f"  Merged: {len(merged)} definitions")

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    out_path = OUTPUT_DIR / f"v{protocol}.json"
    output = []
    for defn in merged:
        entry: dict = {"name": defn.name}
        if defn.packet_id is not None:
            entry["packet_id"] = defn.packet_id
        if defn.direction:
            entry["direction"] = defn.direction
        if defn.description:
            entry["description"] = defn.description
        if defn.fields:
            entry["fields"] = [_to_output(f) for f in defn.fields]
        output.append(entry)
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(output, f, indent=2)
    print(f"  Written to {out_path}")


def ensure_parsed(protocol: int) -> None:
    """Parse protocol docs if the output JSON does not already exist."""
    out_path = OUTPUT_DIR / f"v{protocol}.json"
    if out_path.exists():
        return
    parse_version(protocol)


def main() -> None:
    """Entry point: parse protocol docs for selected versions."""
    parser = argparse.ArgumentParser(description="Parse protocol DOT + JSON docs into structured JSON.")
    parser.add_argument(
        "versions",
        nargs="*",
        help="Protocol numbers to parse (e.g. 924 944). Defaults to all known versions.",
    )
    args = parser.parse_args()

    all_protos = [v.protocol for v in VERSIONS.values()]

    if args.versions:
        selected = []
        for arg in args.versions:
            try:
                proto = int(arg)
            except ValueError:
                print(f"Error: '{arg}' is not a valid protocol number. Known: {', '.join(str(k) for k in all_protos)}")
                sys.exit(1)
            if proto not in all_protos:
                print(f"Error: unknown protocol {proto}. Known: {', '.join(str(k) for k in all_protos)}")
                sys.exit(1)
            selected.append(proto)
    else:
        selected = all_protos

    for protocol in selected:
        parse_version(protocol)
    print("Done.")


if __name__ == "__main__":
    main()
