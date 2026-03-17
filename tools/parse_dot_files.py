#!/usr/bin/env python3
"""Parse DOT protocol documentation files into structured JSON.

Reads .dot files from protocol_docs/<version>/dot/ and outputs
structured JSON to src/endstone_endweave/data/<version>_packets.json.

DOT format:
- Node 0 = packet root, `comment` has metadata
- Child nodes = fields, `label` = name, `comment` has typeName + attributes
- attributes: 512 = leaf/primitive, 8 = list, 16 = list element example
- Edges define parent→child containment
"""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent

DOT_VERSIONS = {
    "r26_u0": ("protocol_docs/r26_u0/dot", "v924_packets.json"),
    "r26_u1": ("protocol_docs/r26_u1/dot", "v944_packets.json"),
}

OUTPUT_DIR = PROJECT_ROOT / "src" / "endstone_endweave" / "data"

# Regex patterns for DOT parsing
NODE_RE = re.compile(
    r'(\d+)\s*\[([^\]]*)\]', re.DOTALL
)
ATTR_RE = re.compile(
    r'(\w+)\s*=\s*"([^"]*)"'
)
EDGE_RE = re.compile(
    r'(\d+)\s*->\s*(\d+)'
)


def parse_dot_file(filepath: Path) -> dict | None:
    """Parse a single .dot file into a structured dict."""
    text = filepath.read_text(encoding="utf-8", errors="replace")

    nodes: dict[int, dict[str, str]] = {}
    edges: list[tuple[int, int]] = []

    # Parse nodes
    for match in NODE_RE.finditer(text):
        node_id = int(match.group(1))
        attrs_str = match.group(2)
        attrs = {}
        for attr_match in ATTR_RE.finditer(attrs_str):
            attrs[attr_match.group(1)] = attr_match.group(2)
        nodes[node_id] = attrs

    # Parse edges
    for match in EDGE_RE.finditer(text):
        edges.append((int(match.group(1)), int(match.group(2))))

    if 0 not in nodes:
        return None  # Not a valid packet DOT

    # Build tree
    children: dict[int, list[int]] = {}
    for parent, child in edges:
        children.setdefault(parent, []).append(child)

    root = nodes[0]
    packet_name = filepath.stem

    def build_field(node_id: int) -> dict:
        node = nodes.get(node_id, {})
        comment = node.get("comment", "")

        # Parse comment for typeName and attributes
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

    # Build fields from root's children
    root_children = children.get(0, [])
    fields = [build_field(cid) for cid in root_children]

    return {
        "name": packet_name,
        "comment": root.get("comment", ""),
        "label": root.get("label", packet_name),
        "fields": fields,
    }


def parse_version(dot_dir: Path) -> list[dict]:
    """Parse all .dot files in a directory."""
    if not dot_dir.exists():
        print(f"  Warning: {dot_dir} does not exist, skipping")
        return []

    packets = []
    for dot_file in sorted(dot_dir.glob("*.dot")):
        result = parse_dot_file(dot_file)
        if result:
            packets.append(result)

    return packets


def main() -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    for version_name, (dot_rel, output_file) in DOT_VERSIONS.items():
        dot_dir = PROJECT_ROOT / dot_rel
        print(f"Parsing {version_name} from {dot_dir}...")
        packets = parse_version(dot_dir)
        print(f"  Parsed {len(packets)} packets")

        out_path = OUTPUT_DIR / output_file
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(packets, f, indent=2)
        print(f"  Written to {out_path}")

    print("Done.")


if __name__ == "__main__":
    main()
