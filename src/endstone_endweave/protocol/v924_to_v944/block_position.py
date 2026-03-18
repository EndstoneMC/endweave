"""Utilities for converting between NetworkBlockPosition (v924) and BlockPos (v944).

NetworkBlockPosition (v924):
  - Inherits BlockPos but stores coordinates * 8 on the wire
  - Wire encoding: X=signed_varint, Y=unsigned_varint, Z=signed_varint
  - Wire values are actual_block_coordinate * 8

BlockPos (v944):
  - Direct block coordinates on the wire (no scaling)
  - Wire encoding: X=signed_varint, Y=signed_varint, Z=signed_varint

From the C++ source:
    NetworkBlockPosition(const BlockPos &block_pos)
        : BlockPos(block_pos.x * 8.0, block_pos.y * 8.0, block_pos.z * 8.0)

Conversion requires BOTH:
  1. Scaling: NetworkBlockPosition wire values are 8x actual, BlockPos wire values are 1x
  2. Y encoding: unsigned varint (v924) vs signed varint (v944)
"""

from __future__ import annotations

from collections.abc import Callable

from endstone_endweave.codec import PacketReader, PacketWriter

PosReader = Callable[[PacketReader], tuple[int, int, int]]
PosWriter = Callable[[PacketWriter, int, int, int], None]

NETWORK_BLOCK_POS_SCALE = 8


def read_network_block_pos(reader: PacketReader) -> tuple[int, int, int]:
    """Read v924 NetworkBlockPosition from wire. Returns scaled wire values."""
    x = reader.read_signed_varint()
    y = reader.read_varint()  # unsigned
    z = reader.read_signed_varint()
    return x, y, z


def write_network_block_pos(writer: PacketWriter, x: int, y: int, z: int) -> None:
    """Write v924 NetworkBlockPosition to wire. Expects scaled wire values."""
    writer.write_signed_varint(x)
    writer.write_varint(y)  # unsigned
    writer.write_signed_varint(z)


def read_block_pos(reader: PacketReader) -> tuple[int, int, int]:
    """Read v944 BlockPos from wire. Returns actual block coordinates."""
    x = reader.read_signed_varint()
    y = reader.read_signed_varint()
    z = reader.read_signed_varint()
    return x, y, z


def write_block_pos(writer: PacketWriter, x: int, y: int, z: int) -> None:
    """Write v944 BlockPos to wire. Expects actual block coordinates."""
    writer.write_signed_varint(x)
    writer.write_signed_varint(y)
    writer.write_signed_varint(z)


def convert_924_to_944(reader: PacketReader, writer: PacketWriter) -> None:
    """Read NetworkBlockPosition (v924), write BlockPos (v944).

    Upscale: server -> client. Divides wire values by 8.
    """
    x, y, z = read_network_block_pos(reader)
    write_block_pos(writer, x // NETWORK_BLOCK_POS_SCALE, y // NETWORK_BLOCK_POS_SCALE, z // NETWORK_BLOCK_POS_SCALE)


def convert_944_to_924(reader: PacketReader, writer: PacketWriter) -> None:
    """Read BlockPos (v944), write NetworkBlockPosition (v924).

    Downscale: client -> server. Multiplies wire values by 8.
    """
    x, y, z = read_block_pos(reader)
    write_network_block_pos(writer, x * NETWORK_BLOCK_POS_SCALE, y * NETWORK_BLOCK_POS_SCALE, z * NETWORK_BLOCK_POS_SCALE)



def read_pos_v924(reader: PacketReader) -> tuple[int, int, int]:
    """Read v924 NetworkBlockPosition and return logical block coordinates."""
    x, y, z = read_network_block_pos(reader)
    return x // NETWORK_BLOCK_POS_SCALE, y // NETWORK_BLOCK_POS_SCALE, z // NETWORK_BLOCK_POS_SCALE


def write_pos_v924(writer: PacketWriter, x: int, y: int, z: int) -> None:
    """Write logical block coordinates as v924 NetworkBlockPosition."""
    write_network_block_pos(writer, x * NETWORK_BLOCK_POS_SCALE, y * NETWORK_BLOCK_POS_SCALE, z * NETWORK_BLOCK_POS_SCALE)


read_pos_v944 = read_block_pos
write_pos_v944 = write_block_pos
