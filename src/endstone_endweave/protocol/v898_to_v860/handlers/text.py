"""Text packet handlers re-exported from v860_to_v898.

The text packet wire format is identical between v860 and v898, so the
same rewrite handlers work in both directions.
"""

from endstone_endweave.protocol.v860_to_v898.handlers.text import (
    rewrite_text_clientbound,
    rewrite_text_serverbound,
)

__all__ = [
    "rewrite_text_clientbound",
    "rewrite_text_serverbound",
]
