"""Shared pytest configuration and fixtures."""

import sys
from pathlib import Path

import pytest

from endstone_endweave.codec.reader import PacketReader
from endstone_endweave.codec.writer import PacketWriter

# Ensure tests/ is on sys.path so `from helpers import ...` works.
sys.path.insert(0, str(Path(__file__).parent))


@pytest.fixture
def writer() -> PacketWriter:
    """Fresh PacketWriter instance."""
    return PacketWriter()


@pytest.fixture
def make_reader() -> type[PacketReader]:
    """PacketReader class for constructing from bytes."""
    return PacketReader
