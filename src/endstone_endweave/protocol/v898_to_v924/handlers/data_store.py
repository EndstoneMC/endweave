"""Data store packet handlers for v898 to v924."""

from endstone_endweave.codec import BOOL, CHANGE_VALUE, DOUBLE_LE, INT_LE, STRING, UVAR_INT, PacketWrapper


def rewrite_serverbound_data_store(wrapper: PacketWrapper) -> None:
    """Strip PathUpdateCount from newer client writes.

    Args:
        wrapper: Packet wrapper for ServerboundDataStore.
    """
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)

    data_type = wrapper.passthrough(UVAR_INT)
    if data_type == 0:
        wrapper.passthrough(DOUBLE_LE)
    elif data_type == 1:
        wrapper.passthrough(BOOL)
    elif data_type == 2:
        wrapper.passthrough(STRING)
    else:
        raise ValueError(f"Invalid data store data type: {data_type}")

    wrapper.passthrough(INT_LE)
    wrapper.read(INT_LE)


def rewrite_clientbound_data_store(wrapper: PacketWrapper) -> None:
    """Append PathUpdateCount for newer clients.

    Args:
        wrapper: Packet wrapper for ClientboundDataStore.
    """
    update_count = wrapper.passthrough(UVAR_INT)
    for _ in range(update_count):
        action_type = wrapper.passthrough(UVAR_INT)
        if action_type == 0:
            wrapper.passthrough(STRING)
            wrapper.passthrough(STRING)
            wrapper.passthrough(STRING)

            data_type = wrapper.passthrough(UVAR_INT)
            if data_type == 0:
                wrapper.passthrough(DOUBLE_LE)
            elif data_type == 1:
                wrapper.passthrough(BOOL)
            elif data_type == 2:
                wrapper.passthrough(STRING)
            else:
                raise ValueError(f"Invalid data store data type: {data_type}")

            wrapper.passthrough(INT_LE)
            wrapper.write(INT_LE, 0)
        elif action_type == 1:
            wrapper.passthrough(STRING)
            wrapper.passthrough(STRING)
            wrapper.passthrough(INT_LE)
            wrapper.passthrough(CHANGE_VALUE)
        elif action_type == 2:
            wrapper.passthrough(STRING)
        else:
            raise ValueError(f"Unknown data store action: {action_type}")
