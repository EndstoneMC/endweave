"""BookEdit packet handler for v898 server <- v924 client (serverbound)."""

from endstone_endweave.codec import BYTE, VAR_INT, PacketWrapper

_ACTION_REPLACE_PAGE = 0
_ACTION_ADD_PAGE = 1
_ACTION_DELETE_PAGE = 2
_ACTION_SWAP_PAGES = 3
_ACTION_FINALIZE = 4


def rewrite_book_edit(wrapper: PacketWrapper) -> None:
    """Rewrite BookEdit from the v924 wire format to the v898 format.

    v924 sends Book Slot (varint) then Operation type (byte) with varint
    page indices; v898 expects Action then Book Slot (both bytes) with
    byte page indices.

    Args:
        wrapper: Packet wrapper for BookEdit.
    """
    book_slot = wrapper.read(VAR_INT)  # Book Slot
    action = wrapper.read(BYTE)  # Operation type

    wrapper.write(BYTE, action)  # Action
    wrapper.write(BYTE, book_slot)  # Book Slot

    if action in (_ACTION_REPLACE_PAGE, _ACTION_ADD_PAGE):
        wrapper.write(BYTE, wrapper.read(VAR_INT))  # Page Index
    elif action == _ACTION_DELETE_PAGE:
        wrapper.write(BYTE, wrapper.read(VAR_INT))  # Page Index
    elif action == _ACTION_SWAP_PAGES:
        wrapper.write(BYTE, wrapper.read(VAR_INT))  # Page Index A
        wrapper.write(BYTE, wrapper.read(VAR_INT))  # Page Index B
    elif action == _ACTION_FINALIZE:
        pass
    else:
        raise ValueError(f"Unknown BookEdit action: {action}")
