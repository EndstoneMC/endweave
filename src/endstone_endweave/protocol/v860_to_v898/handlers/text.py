"""Handler for TextPacket -- v860 server to v898 client."""

from endstone_endweave.codec import (
    BOOL,
    BYTE,
    STRING,
    UVAR_INT,
    PacketWrapper,
)

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
    """Rewrite Text from the v860 wire format to the v898 format.

    Args:
        wrapper: Packet wrapper for Text.
    """
    text_type = wrapper.read(BYTE)
    needs_translation = wrapper.read(BOOL)

    wrapper.write(BOOL, needs_translation)
    if text_type in _TEXT_MESSAGE_ONLY:
        wrapper.write(BYTE, _TEXT_KIND_MESSAGE_ONLY)
        for label in _TEXT_MESSAGE_ONLY[text_type]:
            wrapper.write(STRING, label)
        wrapper.write(BYTE, text_type)
        wrapper.passthrough(STRING)
    elif text_type in _TEXT_AUTHOR_AND_MESSAGE:
        wrapper.write(BYTE, _TEXT_KIND_AUTHOR_AND_MESSAGE)
        for label in _TEXT_AUTHOR_AND_MESSAGE[text_type]:
            wrapper.write(STRING, label)
        wrapper.write(BYTE, text_type)
        wrapper.passthrough(STRING)
        wrapper.passthrough(STRING)
    elif text_type in _TEXT_MESSAGE_AND_PARAMS:
        wrapper.write(BYTE, _TEXT_KIND_MESSAGE_AND_PARAMS)
        for label in _TEXT_MESSAGE_AND_PARAMS[text_type]:
            wrapper.write(STRING, label)
        wrapper.write(BYTE, text_type)
        wrapper.passthrough(STRING)
        parameter_count = wrapper.passthrough(UVAR_INT)
        for _ in range(parameter_count):
            wrapper.passthrough(STRING)
    else:
        raise ValueError(f"Unknown text type: {text_type}")

    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    filtered_message = wrapper.read(STRING)
    wrapper.write(BOOL, filtered_message != "")
    if filtered_message != "":
        wrapper.write(STRING, filtered_message)


def rewrite_text_serverbound(wrapper: PacketWrapper) -> None:
    """Rewrite Text from the v898 wire format to the v860 format.

    Args:
        wrapper: Packet wrapper for Text.
    """
    needs_translation = wrapper.read(BOOL)
    kind = wrapper.read(BYTE)

    if kind == _TEXT_KIND_MESSAGE_ONLY:
        for _ in range(6):
            wrapper.read(STRING)
        text_type = wrapper.read(BYTE)
        wrapper.write(BYTE, text_type)
        wrapper.write(BOOL, needs_translation)
        wrapper.passthrough(STRING)
    elif kind == _TEXT_KIND_AUTHOR_AND_MESSAGE:
        for _ in range(3):
            wrapper.read(STRING)
        text_type = wrapper.read(BYTE)
        wrapper.write(BYTE, text_type)
        wrapper.write(BOOL, needs_translation)
        wrapper.passthrough(STRING)
        wrapper.passthrough(STRING)
    elif kind == _TEXT_KIND_MESSAGE_AND_PARAMS:
        for _ in range(3):
            wrapper.read(STRING)
        text_type = wrapper.read(BYTE)
        wrapper.write(BYTE, text_type)
        wrapper.write(BOOL, needs_translation)
        wrapper.passthrough(STRING)
        parameter_count = wrapper.passthrough(UVAR_INT)
        for _ in range(parameter_count):
            wrapper.passthrough(STRING)
    else:
        raise ValueError(f"Unknown text kind: {kind}")

    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    filtered_message = ""
    if wrapper.read(BOOL):
        filtered_message = wrapper.read(STRING)
    wrapper.write(STRING, filtered_message)
