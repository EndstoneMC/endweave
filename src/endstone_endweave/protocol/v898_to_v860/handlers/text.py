"""Text packet handlers re-exported from v860_to_v898."""

from endstone_endweave.protocol.v860_to_v898.handlers.text import (
    rewrite_text_clientbound,
    rewrite_text_serverbound,
)

__all__ = [
    "rewrite_text_clientbound",
    "rewrite_text_serverbound",
]
