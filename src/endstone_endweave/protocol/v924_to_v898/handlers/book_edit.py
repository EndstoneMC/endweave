"""BookEdit packet handler for v924 server <- v898 client (serverbound)."""

from endstone_endweave.codec import BYTE, STRING, VAR_INT, PacketWrapper

_ACTION_REPLACE_PAGE = 0
_ACTION_ADD_PAGE = 1
_ACTION_DELETE_PAGE = 2
_ACTION_SWAP_PAGES = 3
_ACTION_FINALIZE = 4


def rewrite_book_edit(wrapper: PacketWrapper) -> None:
    """Rewrite BookEdit from the v898 wire format to the v924 format.

    v898 sends Action then Book Slot (both bytes); v924 expects Book Slot
    (varint) then Operation type (byte) with varint page indices.

    Args:
        wrapper: Packet wrapper for BookEdit.
    """
    action = wrapper.read(BYTE)  # Action
    book_slot = wrapper.read(BYTE)  # Book Slot

    wrapper.write(VAR_INT, book_slot)  # Book Slot
    wrapper.write(BYTE, action)  # Operation type

    if action == _ACTION_REPLACE_PAGE:
        wrapper.write(VAR_INT, wrapper.read(BYTE))  # Page Index
        wrapper.passthrough(STRING)  # Page Text
        wrapper.passthrough(STRING)  # Photo Name
    elif action == _ACTION_ADD_PAGE:
        wrapper.write(VAR_INT, wrapper.read(BYTE))  # Page Index
        wrapper.passthrough(STRING)  # Page Text
        wrapper.passthrough(STRING)  # Photo Name
    elif action == _ACTION_DELETE_PAGE:
        wrapper.write(VAR_INT, wrapper.read(BYTE))  # Page Index
    elif action == _ACTION_SWAP_PAGES:
        wrapper.write(VAR_INT, wrapper.read(BYTE))  # Page Index
        wrapper.write(VAR_INT, wrapper.read(BYTE))  # Swap With Index
    elif action == _ACTION_FINALIZE:
        wrapper.passthrough(STRING)  # Title
        wrapper.passthrough(STRING)  # Author
        wrapper.passthrough(STRING)  # XUID
    else:
        raise ValueError(f"Unknown BookEdit action: {action}")
