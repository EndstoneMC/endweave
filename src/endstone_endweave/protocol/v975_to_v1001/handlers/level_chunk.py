"""LevelChunk(58) v975 server -> v1001 client.

Fixes a 1.26.30 ("Chaos Cubed") client that renders the TOP sub-chunks of every
column as AIR. The v944/v975 server streams chunks in sub-chunk-request "Limited"
mode: it sets SubChunkCount to the Limited sentinel and follows it with a uint16
HighestSubChunk cap computed for the OLD (shorter) world height. A 1.26.30 client
honours that cap and treats every sub-chunk above HighestSubChunk as air, so the
upper sections of the column never request their block data and vanish. (Confirmed
in real captures: every Limited LevelChunk carries HighestSubChunk in 1..8, all
strictly below the 16 sub-chunks of the End dimension, so the top is always clipped.)

FIX: rewrite Limited (SubChunkCount = -2, followed by HighestSubChunk) into Limitless
(SubChunkCount = -1, no cap) before forwarding to the v1001 client. The client then
requests the full column and the server fills every section. Packets already sent in
Limitless mode (-1) or with a legacy explicit positive count are left untouched.

Wire layout (gophertunnel v1.57.0 packet.LevelChunk.Marshal, verified byte-identical
at v1.55.2/v944)::

    ChunkPos   Position           (2x Varint32: X, Z)
    Varint32   Dimension
    Varuint32  SubChunkCount      (Limitless = -1 = 0xFFFFFFFF, Limited = -2 = 0xFFFFFFFE)
    if SubChunkCount == Limited:
        Uint16 LE  HighestSubChunk                # <- dropped here when rewriting to Limitless
    Bool       CacheEnabled
    if CacheEnabled:
        Varuint32-length slice of Uint64 BlobHashes
    ByteSlice  RawPayload         (varuint32 len + bytes)

Only the SubChunkCount field (and the dependent HighestSubChunk) changes. Everything
from CacheEnabled onward is copied byte-for-byte via passthrough_all, so the cache
blob hashes and the raw chunk payload are never parsed.

This is registered on the v975 -> v1001 (clientbound) delta only: a real v975-terminal
client honours the Limited cap correctly for the matching world height, so the rewrite
is target-version specific.
"""

from endstone_endweave.codec import UVAR_INT, USHORT_LE, VAR_INT, PacketWrapper

# SubChunkCount request-mode sentinels (gophertunnel protocol.SubChunkRequestMode*,
# math.MaxUint32 - iota): Limitless = -1, Limited = -2 as unsigned uint32.
_SUB_CHUNK_REQUEST_MODE_LIMITLESS = 0xFFFFFFFF  # 4294967295 (-1): request full column, no cap
_SUB_CHUNK_REQUEST_MODE_LIMITED = 0xFFFFFFFE  # 4294967294 (-2): HighestSubChunk cap follows


def rewrite_level_chunk(wrapper: PacketWrapper) -> None:
    """LevelChunk (58): rewrite Limited sub-chunk mode into Limitless.

    Passes every field through unchanged except SubChunkCount. When the count is
    the Limited sentinel, the trailing uint16 HighestSubChunk cap is dropped and
    the count is rewritten to the Limitless sentinel, so the 1.26.30 client
    requests the full column instead of treating sections above the cap as air.
    Limitless and legacy explicit-count packets are forwarded byte-for-byte.

    Args:
        wrapper: Packet wrapper for a clientbound LevelChunk (FullChunkData) packet.
    """
    wrapper.passthrough(VAR_INT)  # Chunk X
    wrapper.passthrough(VAR_INT)  # Chunk Z
    wrapper.passthrough(VAR_INT)  # Dimension

    sub_chunk_count = wrapper.read(UVAR_INT)
    if sub_chunk_count == _SUB_CHUNK_REQUEST_MODE_LIMITED:
        wrapper.read(USHORT_LE)  # HighestSubChunk - used for older clients
        wrapper.write(UVAR_INT, _SUB_CHUNK_REQUEST_MODE_LIMITLESS)
    else:
        wrapper.write(UVAR_INT, sub_chunk_count)

    wrapper.passthrough_all()
