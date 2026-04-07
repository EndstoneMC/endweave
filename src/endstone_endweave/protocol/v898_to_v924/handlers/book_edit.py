"""BookEdit packet handler for v898 server <- v924 client (serverbound)."""

from ....codec import BYTE, VAR_INT, BookEditActionType, PacketWrapper


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

    if action in (BookEditActionType.REPLACE_PAGE, BookEditActionType.ADD_PAGE):
        wrapper.write(BYTE, wrapper.read(VAR_INT))  # Page Index
    elif action == BookEditActionType.DELETE_PAGE:
        wrapper.write(BYTE, wrapper.read(VAR_INT))  # Page Index
    elif action == BookEditActionType.SWAP_PAGES:
        wrapper.write(BYTE, wrapper.read(VAR_INT))  # Page Index A
        wrapper.write(BYTE, wrapper.read(VAR_INT))  # Page Index B
    elif action == BookEditActionType.FINALIZE:
        pass
    else:
        raise ValueError(f"Unknown BookEdit action: {action}")
