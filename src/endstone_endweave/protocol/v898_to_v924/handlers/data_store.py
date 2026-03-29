"""Data store packet handlers for v898 to v924."""

from endstone_endweave.codec import BOOL, CHANGE_VALUE, DOUBLE_LE, INT_LE, STRING, UVAR_INT, PacketWrapper


def rewrite_serverbound_data_store(wrapper: PacketWrapper) -> None:
    """Strip PathUpdateCount from newer client writes.

    Args:
        wrapper: Packet wrapper for ServerboundDataStore.
    """
    wrapper.passthrough(STRING)  # DataStoreName
    wrapper.passthrough(STRING)  # Property
    wrapper.passthrough(STRING)  # Key

    data_type = wrapper.passthrough(UVAR_INT)  # DataType
    if data_type == 0:
        wrapper.passthrough(DOUBLE_LE)  # Double
    elif data_type == 1:
        wrapper.passthrough(BOOL)  # Bool
    elif data_type == 2:
        wrapper.passthrough(STRING)  # String
    else:
        raise ValueError(f"Invalid data store data type: {data_type}")

    wrapper.passthrough(INT_LE)  # ChangeType
    wrapper.read(INT_LE)  # PathUpdateCount (strip for v898)


def rewrite_clientbound_data_store(wrapper: PacketWrapper) -> None:
    """Append PathUpdateCount for newer clients.

    Args:
        wrapper: Packet wrapper for ClientboundDataStore.
    """
    update_count = wrapper.passthrough(UVAR_INT)  # Updates
    for _ in range(update_count):
        action_type = wrapper.passthrough(UVAR_INT)  # ActionType
        if action_type == 0:
            wrapper.passthrough(STRING)  # DataStoreName
            wrapper.passthrough(STRING)  # Property
            wrapper.passthrough(STRING)  # Key

            data_type = wrapper.passthrough(UVAR_INT)  # DataType
            if data_type == 0:
                wrapper.passthrough(DOUBLE_LE)  # Double
            elif data_type == 1:
                wrapper.passthrough(BOOL)  # Bool
            elif data_type == 2:
                wrapper.passthrough(STRING)  # String
            else:
                raise ValueError(f"Invalid data store data type: {data_type}")

            wrapper.passthrough(INT_LE)  # ChangeType
            wrapper.write(INT_LE, 0)  # PathUpdateCount
        elif action_type == 1:
            wrapper.passthrough(STRING)  # DataStoreName
            wrapper.passthrough(STRING)  # Property
            wrapper.passthrough(INT_LE)  # ChangeType
            wrapper.passthrough(CHANGE_VALUE)  # ChangeValue
        elif action_type == 2:
            wrapper.passthrough(STRING)  # DataStoreName
        else:
            raise ValueError(f"Unknown data store action: {action_type}")
