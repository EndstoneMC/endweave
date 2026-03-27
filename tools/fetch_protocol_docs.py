#!/usr/bin/env python3
"""Fetch protocol documentation from the BedrockProtocol repository.

Downloads DOT and JSON files for protocol versions into protocol_docs/.

Usage:
    uv run tools/fetch_protocol_docs.py              # fetch all known versions
    uv run tools/fetch_protocol_docs.py 924 944      # fetch specific versions
"""

import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "src"))
from endstone_endweave.protocol.versions import VERSIONS

REPO_URL = "https://github.com/Mojang/bedrock-protocol-docs.git"

OUTPUT_DIR = Path(__file__).resolve().parent.parent / "protocol_docs"


def fetch_version(protocol: int, branch: str) -> None:
    """Fetch protocol docs for a single version from the BedrockProtocol repo.

    Shallow-clones the given branch into a temp directory, then copies the
    dot/, json/, and changelog files into protocol_docs/v<protocol>/.

    Args:
        protocol: Protocol version number (e.g. 924).
        branch: Git branch to clone (e.g. "r/26_u0").
    """
    dest = OUTPUT_DIR / f"v{protocol}"
    if dest.exists():
        print(f"  {dest} already exists, skipping")
        return

    with tempfile.TemporaryDirectory() as tmp:
        print(f"  Cloning branch {branch}...")
        clone_cmd = [
            "git", "clone", "--depth", "1", "--branch", branch,
            "--single-branch", REPO_URL, tmp,
        ]
        result = subprocess.run(clone_cmd, capture_output=True, text=True)
        if result.returncode != 0:
            # Retry with uppercase variant (Mojang uses e.g. r/21_U12 not r/21_u12)
            parts = branch.rsplit("_", 1)
            if len(parts) == 2:
                alt_branch = f"{parts[0]}_{parts[1].upper()}"
                print(f"  Branch {branch} not found, trying {alt_branch}...")
                clone_cmd[5] = alt_branch
                subprocess.run(clone_cmd, check=True, capture_output=True, text=True)
            else:
                result.check_returncode()

        dest.mkdir(parents=True, exist_ok=True)

        tmp_path = Path(tmp)
        for subdir in ("dot", "json"):
            src = tmp_path / subdir
            if src.exists():
                shutil.copytree(src, dest / subdir)
                print(f"  Copied {subdir}/ -> {dest / subdir}")

        # Also copy changelog if present
        for changelog in tmp_path.glob("changelog*"):
            shutil.copy2(changelog, dest / changelog.name)
            print(f"  Copied {changelog.name}")


def main() -> None:
    """Entry point: parse CLI args and fetch protocol docs for selected versions."""
    parser = argparse.ArgumentParser(description="Fetch protocol docs from BedrockProtocol repo.")
    parser.add_argument(
        "versions",
        nargs="*",
        help="Protocol numbers to fetch (e.g. 924 944). Defaults to all known versions.",
    )
    args = parser.parse_args()

    # Build protocol -> branch mapping from versions registry
    all_versions: dict[int, str] = {}
    for v in VERSIONS.values():
        tag = v.release_tag
        all_versions[v.protocol] = f"r/{tag.replace('r', '', 1)}"

    if args.versions:
        selected: dict[int, str] = {}
        for arg in args.versions:
            try:
                proto = int(arg)
            except ValueError:
                known = ", ".join(str(k) for k in all_versions)
                print(f"Error: '{arg}' is not a valid protocol number. Known: {known}")
                sys.exit(1)
            if proto not in all_versions:
                print(f"Error: unknown protocol {proto}. Known: {', '.join(str(k) for k in all_versions)}")
                sys.exit(1)
            selected[proto] = all_versions[proto]
    else:
        selected = all_versions

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    for protocol, branch in selected.items():
        print(f"Fetching v{protocol} ({branch})...")
        fetch_version(protocol, branch)
    print("Done.")


if __name__ == "__main__":
    main()
