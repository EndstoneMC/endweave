"""Text packet handlers for v924 to v898."""

from endstone_endweave.codec import BOOL, BYTE, STRING, UVAR_INT, PacketWrapper

_TEXT_KIND_MESSAGE_ONLY = 0
_TEXT_KIND_AUTHOR_AND_MESSAGE = 1
_TEXT_KIND_MESSAGE_AND_PARAMS = 2

_TEXT_MESSAGE_ONLY = {
    0: ("raw", "tip", "systemMessage", "textObjectWhisper", "textObjectAnnouncement", "textObject"),
    5: ("raw", "tip", "systemMessage", "textObjectWhisper", "textObjectAnnouncement", "textObject"),
    6: ("raw", "tip", "systemMessage", "textObjectWhisper", "textObjectAnnouncement", "textObject"),
    9: ("raw", "tip", "systemMessage", "textObjectWhisper", "textObjectAnnouncement", "textObject"),
    10: ("raw", "tip", "systemMessage", "textObjectWhisper", "textObjectAnnouncement", "textObject"),
    11: ("raw", "tip", "systemMessage", "textObjectWhisper", "textObjectAnnouncement", "textObject"),
}
_TEXT_AUTHOR_AND_MESSAGE = {
    1: ("chat", "whisper", "announcement"),
    7: ("chat", "whisper", "announcement"),
    8: ("chat", "whisper", "announcement"),
}
_TEXT_MESSAGE_AND_PARAMS = {
    2: ("translate", "popup", "jukeboxPopup"),
    3: ("translate", "popup", "jukeboxPopup"),
    4: ("translate", "popup", "jukeboxPopup"),
}


def rewrite_text_clientbound(wrapper: PacketWrapper) -> None:
    """Rewrite Text from the v924 wire format to the v898 format.

    Args:
        wrapper: Packet wrapper for Text.
    """
    wrapper.passthrough(BOOL)  # Needs Translation
    kind = wrapper.read(BYTE)
    wrapper.write(BYTE, kind)

    if kind == _TEXT_KIND_MESSAGE_ONLY:
        text_type = wrapper.read(BYTE)
        for label in _TEXT_MESSAGE_ONLY[text_type]:
            wrapper.write(STRING, label)
        wrapper.write(BYTE, text_type)
        wrapper.passthrough(STRING)
    elif kind == _TEXT_KIND_AUTHOR_AND_MESSAGE:
        text_type = wrapper.read(BYTE)
        for label in _TEXT_AUTHOR_AND_MESSAGE[text_type]:
            wrapper.write(STRING, label)
        wrapper.write(BYTE, text_type)
        wrapper.passthrough(STRING)
        wrapper.passthrough(STRING)
    elif kind == _TEXT_KIND_MESSAGE_AND_PARAMS:
        text_type = wrapper.read(BYTE)
        for label in _TEXT_MESSAGE_AND_PARAMS[text_type]:
            wrapper.write(STRING, label)
        wrapper.write(BYTE, text_type)
        wrapper.passthrough(STRING)
        parameter_count = wrapper.passthrough(UVAR_INT)
        for _ in range(parameter_count):
            wrapper.passthrough(STRING)
    else:
        raise ValueError(f"Unknown text kind: {kind}")

    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    has_filtered_message = wrapper.read(BOOL)
    if has_filtered_message:
        wrapper.write(BOOL, True)
        wrapper.passthrough(STRING)
    else:
        wrapper.write(BOOL, False)


def rewrite_text_serverbound(wrapper: PacketWrapper) -> None:
    """Rewrite Text from the v898 wire format to the v924 format.

    Args:
        wrapper: Packet wrapper for Text.
    """
    wrapper.passthrough(BOOL)  # Needs Translation
    kind = wrapper.read(BYTE)
    wrapper.write(BYTE, kind)

    if kind == _TEXT_KIND_MESSAGE_ONLY:
        for _ in range(6):
            wrapper.read(STRING)
        wrapper.passthrough(BYTE)
        wrapper.passthrough(STRING)
    elif kind == _TEXT_KIND_AUTHOR_AND_MESSAGE:
        for _ in range(3):
            wrapper.read(STRING)
        wrapper.passthrough(BYTE)
        wrapper.passthrough(STRING)
        wrapper.passthrough(STRING)
    elif kind == _TEXT_KIND_MESSAGE_AND_PARAMS:
        for _ in range(3):
            wrapper.read(STRING)
        wrapper.passthrough(BYTE)
        wrapper.passthrough(STRING)
        parameter_count = wrapper.passthrough(UVAR_INT)
        for _ in range(parameter_count):
            wrapper.passthrough(STRING)
    else:
        raise ValueError(f"Unknown text kind: {kind}")

    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    has_filtered_message = wrapper.passthrough(BOOL)
    if has_filtered_message:
        wrapper.passthrough(STRING)
