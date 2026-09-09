"""Top-level pytest fixtures shared across the entire suite.

Per-area fixtures live in nearer conftests so the global namespace stays small
and intent-focused.
"""

from __future__ import annotations

from unittest.mock import MagicMock

import pytest


@pytest.fixture
def mock_logger() -> MagicMock:
    """Logger double — assert against .error/.debug/etc. without real I/O."""
    return MagicMock()
