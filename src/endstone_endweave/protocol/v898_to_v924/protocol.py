"""Protocol factory for v898 (1.21.130) server <- v924 (1.26.0) client."""

from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.packet_ids import PacketId
from endstone_endweave.protocol.v898_to_v924.handlers.data_store import (
    rewrite_clientbound_data_store,
    rewrite_serverbound_data_store,
)
from endstone_endweave.protocol.v898_to_v924.handlers.gameplay import (
    rewrite_camera_aim_assist_presets,
    rewrite_start_game,
)
from endstone_endweave.protocol.v924_to_v898.handlers.login import (
    rewrite_login,
    rewrite_request_network_settings,
)
from endstone_endweave.protocol.v924_to_v898.handlers.text import (
    rewrite_text_clientbound,
    rewrite_text_serverbound,
)

SERVER_PROTOCOL = 898
CLIENT_PROTOCOL = 924


def create_protocol() -> Protocol:
    """Create a protocol for v898 server <- v924 client."""
    protocol = Protocol(server_protocol=SERVER_PROTOCOL, client_protocol=CLIENT_PROTOCOL)

    protocol.register_serverbound(PacketId.REQUEST_NETWORK_SETTINGS, rewrite_request_network_settings)
    protocol.register_serverbound(PacketId.LOGIN, rewrite_login)
    protocol.register_serverbound(PacketId.TEXT, rewrite_text_clientbound)
    protocol.register_serverbound(PacketId.SERVERBOUND_DATA_STORE, rewrite_serverbound_data_store)

    protocol.register_clientbound(PacketId.START_GAME, rewrite_start_game)
    protocol.register_clientbound(PacketId.TEXT, rewrite_text_serverbound)
    protocol.register_clientbound(PacketId.CLIENTBOUND_DATA_STORE, rewrite_clientbound_data_store)
    protocol.register_clientbound(PacketId.CAMERA_AIM_ASSIST_PRESETS, rewrite_camera_aim_assist_presets)

    return protocol
