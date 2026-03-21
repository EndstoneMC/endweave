"""ItemInstance dataclass and codec type."""

from dataclasses import dataclass

from endstone_endweave.codec.reader import PacketReader
from endstone_endweave.codec.types.primitives import Type
from endstone_endweave.codec.writer import PacketWriter


@dataclass
class ItemInstance:
    """Deserialized Bedrock ItemInstance.

    Attributes:
        network_id: Item network ID. Zero means air (empty slot).
        count: Stack size.
        aux_value: Metadata/damage value for the item.
        has_net_id: Whether the item carries a stack network ID.
        stack_net_id: Stack network ID for inventory transaction tracking.
        block_runtime_id: Runtime ID of the block form of this item.
        user_data: Raw extra data blob (NBT, canPlace/canBreak lists, etc.).
    """

    network_id: int = 0
    count: int = 0
    aux_value: int = 0
    has_net_id: bool = False
    stack_net_id: int = 0
    block_runtime_id: int = 0
    user_data: bytes = b""


class _ItemInstance(Type[ItemInstance]):
    """Bedrock ItemInstance codec -- full deserialization.

    Wire format::

        varint32  NetworkID        (0 = air, terminates early)
        uint16    Count
        uvarint32 MetadataValue
        bool      HasNetID
        varint32  StackNetworkID   (only if HasNetID)
        varint32  BlockRuntimeID
        uvarint32 extraDataLength
        bytes     extraData
    """

    def read(self, reader: PacketReader) -> ItemInstance:
        network_id = reader.read_varint()
        if network_id == 0:
            return ItemInstance(network_id=0)
        count = reader.read_ushort_le()
        aux_value = reader.read_uvarint()
        has_net_id = reader.read_bool()
        stack_net_id = reader.read_varint() if has_net_id else 0
        block_runtime_id = reader.read_varint()
        extra_len = reader.read_uvarint()
        user_data = reader.read_bytes(extra_len)
        return ItemInstance(
            network_id=network_id,
            count=count,
            aux_value=aux_value,
            has_net_id=has_net_id,
            stack_net_id=stack_net_id,
            block_runtime_id=block_runtime_id,
            user_data=user_data,
        )

    def write(self, writer: PacketWriter, value: ItemInstance) -> None:
        writer.write_varint(value.network_id)
        if value.network_id == 0:
            return
        writer.write_ushort_le(value.count)
        writer.write_uvarint(value.aux_value)
        writer.write_bool(value.has_net_id)
        if value.has_net_id:
            writer.write_varint(value.stack_net_id)
        writer.write_varint(value.block_runtime_id)
        writer.write_uvarint(len(value.user_data))
        writer.write_bytes(value.user_data)


ITEM_INSTANCE = _ItemInstance()
