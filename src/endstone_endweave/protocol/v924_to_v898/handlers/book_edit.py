"""BookEdit packet handler for v924 server <- v898 client (serverbound)."""

from ....codec import BYTE, VAR_INT, BookEditActionType, PacketWrapper


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

    if action in (BookEditActionType.REPLACE_PAGE, BookEditActionType.ADD_PAGE):
        wrapper.write(VAR_INT, wrapper.read(BYTE))  # Page Index
    elif action == BookEditActionType.DELETE_PAGE:
        wrapper.write(VAR_INT, wrapper.read(BYTE))  # Page Index
    elif action == BookEditActionType.SWAP_PAGES:
        wrapper.write(VAR_INT, wrapper.read(BYTE))  # Page Index
        wrapper.write(VAR_INT, wrapper.read(BYTE))  # Swap With Index
    elif action == BookEditActionType.FINALIZE:
        pass
    else:
        raise ValueError(f"Unknown BookEdit action: {action}")
