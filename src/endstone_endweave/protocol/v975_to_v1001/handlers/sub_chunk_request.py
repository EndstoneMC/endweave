"""Serverbound SubChunkRequest(175) handler for the v975 <- v1001 delta.

A 1.26.30 client sends SubChunkRequest during chunk streaming right after spawn.
Its wire format changed at v1001 in three ways (gophertunnel v1.56.2 vs v1.57.0
sub_chunk_request.go):

  v975 (OLD, what the v944/v975 server reads)::
      varint32   Dimension
      SubChunkPos Position          (3x varint32: X, Y, Z)
      uint32 LE  OffsetCount        (SliceUint32Length -- FIXED 4-byte prefix)
      OffsetCount x SubChunkOffset  (each = 3 signed bytes: dx, dy, dz)

  v1001 (NEW, what a 1.26.30 client sends)::
      varint32   Dimension
      uvarint32  OffsetCount        (Slice -- varint prefix)
      OffsetCount x SubChunkOffset  (each = 3 signed bytes, unchanged)
      int32 LE   Position X
      int32 LE   Position Y
      int32 LE   Position Z

So at v1001 the Position moved to AFTER the Offsets, its three coords switched
from varint32 to fixed int32 LE, and the Offsets length prefix narrowed from a
fixed uint32 to a varuint32. The SubChunkOffset element (3 signed bytes) is
unchanged. Passed through untranslated, the v944 server reads the offset varint
count as a fixed uint32 (and the rest misaligns), so this serverbound handler
reads the v1001 form and re-emits the v975 form.

v944 == v975 for this packet (it changed only at v1001), so the v975 output passes
straight through the v944_to_v975 layer to the server.
"""

from endstone_endweave.codec import INT_LE, UINT_LE, UVAR_INT, VAR_INT, PacketWrapper

_OFFSET_SIZE = 3  # SubChunkOffset = 3 signed bytes (dx, dy, dz); unchanged across versions


def rewrite_sub_chunk_request(wrapper: PacketWrapper) -> None:
    """SubChunkRequest (175): convert the v1001 wire to the v975 wire.

    Args:
        wrapper: Packet wrapper for a serverbound SubChunkRequestPacket.
    """
    wrapper.passthrough(VAR_INT)  # Dimension (unchanged)

    # Read v1001 NEW: Offsets (varuint32 count + N x 3 bytes), then Position (3x int32 LE).
    offset_count = wrapper.read(UVAR_INT)
    offset_bytes = wrapper.reader.read_bytes(offset_count * _OFFSET_SIZE)
    pos_x = wrapper.read(INT_LE)
    pos_y = wrapper.read(INT_LE)
    pos_z = wrapper.read(INT_LE)

    # Write v975 OLD: Position (3x varint32), then Offsets (uint32 LE count + the same bytes).
    wrapper.write(VAR_INT, pos_x)
    wrapper.write(VAR_INT, pos_y)
    wrapper.write(VAR_INT, pos_z)
    wrapper.write(UINT_LE, offset_count)
    wrapper.writer.write_bytes(offset_bytes)
