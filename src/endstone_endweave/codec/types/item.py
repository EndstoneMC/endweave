"""ItemInstance dataclass and codec types (v944 NetworkItemStackDescriptor and v975 cerealizer)."""

from dataclasses import dataclass

from ..reader import PacketReader
from ..writer import PacketWriter
from .primitives import Type


@dataclass
class ItemInstance:
    """Deserialized Bedrock ItemInstance.

    Attributes:
        network_id: Item network ID. Zero means air (empty slot).
        count: Stack size.
        aux_value: Metadata/damage value for the item.
        has_net_id: Whether the item carries a stack network ID.
        stack_net_id: Stack network ID for inventory transaction tracking.
        net_id_variant: ItemStackNetIdVariant case (0 = server net id, 1 = request id, 2 = legacy request id).
                        v944 only ever uses 0; preserved for round-trips through v975.
        block_runtime_id: Runtime ID of the block form of this item.
        user_data: Raw extra data blob (NBT, canPlace/canBreak lists, etc.).
    """

    network_id: int = 0
    count: int = 0
    aux_value: int = 0
    has_net_id: bool = False
    stack_net_id: int = 0
    net_id_variant: int = 0
    block_runtime_id: int = 0
    user_data: bytes = b""


class _ItemInstanceType(Type["ItemInstance"]):
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


ITEM_INSTANCE = _ItemInstanceType()


class _ItemInstanceV975Type(Type["ItemInstance"]):
    """v975 cerealizer<NetworkItemStackDescriptor>::SerializedData codec.

    Wire format (no air shortcut: all fields always present)::

        int16     Id
        uint16    StackSize          (max 64)
        uvarint32 AuxValue           (max 32767)
        bool      HasNetIdVariant
        uvarint32 VariantType        (only if HasNetIdVariant; 0/1/2)
        varint32  StackNetId         (only if HasNetIdVariant)
        uvarint32 BlockRuntimeId
        uvarint32 UserDataLength
        bytes     UserData
    """

    def read(self, reader: PacketReader) -> ItemInstance:
        network_id = reader.read_short_le()
        count = reader.read_ushort_le()
        aux_value = reader.read_uvarint()
        has_net_id = reader.read_bool()
        net_id_variant = 0
        stack_net_id = 0
        if has_net_id:
            net_id_variant = reader.read_uvarint()
            stack_net_id = reader.read_varint()
        block_runtime_id = reader.read_uvarint()
        extra_len = reader.read_uvarint()
        user_data = reader.read_bytes(extra_len)
        return ItemInstance(
            network_id=network_id,
            count=count,
            aux_value=aux_value,
            has_net_id=has_net_id,
            stack_net_id=stack_net_id,
            net_id_variant=net_id_variant,
            block_runtime_id=block_runtime_id,
            user_data=user_data,
        )

    def write(self, writer: PacketWriter, value: ItemInstance) -> None:
        writer.write_short_le(value.network_id)
        writer.write_ushort_le(value.count)
        writer.write_uvarint(value.aux_value)
        writer.write_bool(value.has_net_id)
        if value.has_net_id:
            writer.write_uvarint(value.net_id_variant)
            writer.write_varint(value.stack_net_id)
        writer.write_uvarint(value.block_runtime_id)
        writer.write_uvarint(len(value.user_data))
        writer.write_bytes(value.user_data)


ITEM_INSTANCE_V975 = _ItemInstanceV975Type()
