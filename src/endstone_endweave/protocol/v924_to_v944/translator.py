"""Protocol translator factory for v924 (r26_u0) server <- v944 (r26_u1) client."""

from __future__ import annotations

from endstone_endweave.protocol.base import ProtocolTranslator
from endstone_endweave.protocol.v924_to_v944.handlers.auth_input import rewrite_auth_input
from endstone_endweave.protocol.v924_to_v944.handlers.block_pos import register_block_pos_handlers
from endstone_endweave.protocol.v924_to_v944.handlers.command_block import rewrite_command_block_update
from endstone_endweave.protocol.v924_to_v944.handlers.login import rewrite_login, rewrite_request_network_settings
from endstone_endweave.protocol.v924_to_v944.handlers.map_item_data import rewrite_map_item_data
from endstone_endweave.protocol.v924_to_v944.handlers.start_game import rewrite_start_game
from endstone_endweave.protocol.v924_to_v944.handlers.structure_block import rewrite_structure_block_update
from endstone_endweave.protocol.v924_to_v944.handlers.sub_chunk import rewrite_sub_chunk_blocks
from endstone_endweave.protocol.v924_to_v944.packet_ids import PacketId


def create_v924_to_v944() -> ProtocolTranslator:
    """Create a translator for v924 server <- v944 client."""
    t = ProtocolTranslator(server_protocol=924, client_protocol=944)

    # Login
    t.register_serverbound(PacketId.REQUEST_NETWORK_SETTINGS, rewrite_request_network_settings)
    t.register_serverbound(PacketId.LOGIN, rewrite_login)

    # Complex manual handlers
    t.register_serverbound(PacketId.COMMAND_BLOCK_UPDATE, rewrite_command_block_update)
    t.register_serverbound(PacketId.STRUCTURE_BLOCK_UPDATE, rewrite_structure_block_update)
    t.register_serverbound(PacketId.PLAYER_AUTH_INPUT, rewrite_auth_input)
    t.register_clientbound(PacketId.START_GAME, rewrite_start_game)
    t.register_clientbound(PacketId.MAP_DATA, rewrite_map_item_data)
    t.register_clientbound(PacketId.UPDATE_SUB_CHUNK_BLOCKS, rewrite_sub_chunk_blocks)

    # Typed block-position packets (13 packets)
    register_block_pos_handlers(t)

    # Cancel new v944 serverbound packets unknown to v924 (v924 EndId = 340)
    t.cancel_serverbound(
        *range(PacketId.RESOURCE_PACKS_READY_FOR_VALIDATION, PacketId.CLIENTBOUND_ATTRIBUTE_LAYER_SYNC + 1)
    )

    return t
