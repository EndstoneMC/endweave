#!/usr/bin/env python3
"""Fetch protocol documentation from the BedrockProtocol repository.

Downloads DOT and JSON files for two protocol versions into protocol_docs/.
"""

from __future__ import annotations

import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_URL = "https://github.com/Mojang/bedrock-protocol-docs.git"

VERSIONS = {
    "r26_u0": "r/26_u0",
    "r26_u1": "r/26_u1",
}

OUTPUT_DIR = Path(__file__).resolve().parent.parent / "protocol_docs"


def fetch_version(name: str, branch: str) -> None:
    dest = OUTPUT_DIR / name
    if dest.exists():
        print(f"  {dest} already exists, skipping")
        return

    with tempfile.TemporaryDirectory() as tmp:
        print(f"  Cloning branch {branch}...")
        subprocess.run(
            [
                "git", "clone",
                "--depth", "1",
                "--branch", branch,
                "--single-branch",
                REPO_URL,
                tmp,
            ],
            check=True,
            capture_output=True,
            text=True,
        )

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
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    for name, branch in VERSIONS.items():
        print(f"Fetching {name} ({branch})...")
        fetch_version(name, branch)
    print("Done.")


if __name__ == "__main__":
    main()
