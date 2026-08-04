"""Serverbound ClientCacheBlobStatus(135) handler for the v975 <- v1001 delta.

The chunk-cache blob-status packet a 1.26.30 client sends after spawn reordered its
fields at v1001:

  v975 (OLD, what the v944/v975 server reads)::
      uvarint32 MissCount
      uvarint32 HitCount
      MissCount x uint64 LE   (miss hashes)
      HitCount  x uint64 LE   (hit hashes)
    -- both counts first, then both hash arrays (gophertunnel v1.56.2:
       Varuint32(missLen); Varuint32(hitLen); FuncSliceOfLen(miss); FuncSliceOfLen(hit)).

  v1001 (NEW, what a 1.26.30 client sends)::
      uvarint32 MissCount
      MissCount x uint64 LE   (miss hashes)
      uvarint32 HitCount
      HitCount  x uint64 LE   (hit hashes)
    -- each count immediately precedes its array (gophertunnel v1.57.0:
       FuncSlice(MissHashes, Uint64); FuncSlice(HitHashes, Uint64)).

Passed through untranslated, the v944 server reads the first miss-hash's low bytes
as HitCount, gets a garbage (huge) count, and overflows -- the observed
"BinaryStream read() overflow ... readNoHeader failed! packetId: 135"
PACKET_MALFORMED that terminated the connection right after spawn.

This serverbound handler reads the v1001 (interleaved) form and re-emits the v975
(counts-first) form. v944 == v975 for this packet (it changed only at v1001), so the
v975 output passes straight through the v944_to_v975 layer to the server.
"""

from endstone_endweave.codec import UVAR_INT, PacketWrapper


def rewrite_client_cache_blob_status(wrapper: PacketWrapper) -> None:
    """ClientCacheBlobStatus (135): convert the v1001 (interleaved) wire to v975 (counts-first).

    Args:
        wrapper: Packet wrapper for a serverbound ClientCacheBlobStatusPacket.
    """
    # Read v1001 NEW: MissCount + miss hashes, then HitCount + hit hashes.
    miss_count = wrapper.read(UVAR_INT)
    miss_hashes = wrapper.reader.read_bytes(miss_count * 8)
    hit_count = wrapper.read(UVAR_INT)
    hit_hashes = wrapper.reader.read_bytes(hit_count * 8)

    # Write v975 OLD: both counts first, then both hash arrays (each hash is an
    # 8-byte uint64 LE, byte-identical between versions -- copy the blobs verbatim).
    wrapper.write(UVAR_INT, miss_count)
    wrapper.write(UVAR_INT, hit_count)
    wrapper.writer.write_bytes(miss_hashes)
    wrapper.writer.write_bytes(hit_hashes)
