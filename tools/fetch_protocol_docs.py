#!/usr/bin/env python3
"""Fetch protocol documentation from the BedrockProtocol repository.

Downloads DOT and JSON files for protocol versions into protocol_docs/.

Usage:
    python tools/fetch_protocol_docs.py              # fetch all known versions
    python tools/fetch_protocol_docs.py r26_u0 r26_u1  # fetch specific versions
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


def fetch_version(name: str, branch: str) -> None:
    """Fetch protocol docs for a single version from the BedrockProtocol repo.

    Shallow-clones the given branch into a temp directory, then copies the
    dot/, json/, and changelog files into protocol_docs/<name>/.

    Args:
        name: Release tag used as the output subdirectory name (e.g. "r26_u0").
        branch: Git branch to clone (e.g. "r/26_u0").
    """
    dest = OUTPUT_DIR / name
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
        help="Release tags to fetch (e.g. r26_u0 r26_u1). Defaults to all known versions.",
    )
    args = parser.parse_args()

    # Build tag -> branch mapping from versions registry
    all_versions = {v.release_tag: f"r/{v.release_tag.replace('r', '', 1)}" for v in VERSIONS.values()}

    if args.versions:
        selected = {}
        for tag in args.versions:
            if tag not in all_versions:
                print(f"Error: unknown version tag '{tag}'. Known: {', '.join(all_versions)}")
                sys.exit(1)
            selected[tag] = all_versions[tag]
    else:
        selected = all_versions

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    for name, branch in selected.items():
        print(f"Fetching {name} ({branch})...")
        fetch_version(name, branch)
    print("Done.")


if __name__ == "__main__":
    main()
