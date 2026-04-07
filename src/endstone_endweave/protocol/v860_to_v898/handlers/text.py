"""Handler for TextPacket -- v860 server to v898 client."""

from ....codec import (
    BOOL,
    BYTE,
    STRING,
    UVAR_INT,
    OptionalType,
    PacketWrapper,
    TextPacketBodyType,
    TextPacketType,
    enum_to_label,
)

_MESSAGE_ONLY_TYPES = (
    TextPacketType.RAW,
    TextPacketType.TIP,
    TextPacketType.SYSTEM_MESSAGE,
    TextPacketType.TEXT_OBJECT_WHISPER,
    TextPacketType.TEXT_OBJECT_ANNOUNCEMENT,
    TextPacketType.TEXT_OBJECT,
)
_AUTHOR_AND_MESSAGE_TYPES = (
    TextPacketType.CHAT,
    TextPacketType.WHISPER,
    TextPacketType.ANNOUNCEMENT,
)
_MESSAGE_AND_PARAMS_TYPES = (
    TextPacketType.TRANSLATE,
    TextPacketType.POPUP,
    TextPacketType.JUKEBOX_POPUP,
)

_labels = enum_to_label(TextPacketType)


def _build_label_group(types: tuple[TextPacketType, ...]) -> dict[int, tuple[str, ...]]:
    labels = tuple(_labels[t] for t in types)
    return {t: labels for t in types}


_TEXT_MESSAGE_ONLY = _build_label_group(_MESSAGE_ONLY_TYPES)
_TEXT_AUTHOR_AND_MESSAGE = _build_label_group(_AUTHOR_AND_MESSAGE_TYPES)
_TEXT_MESSAGE_AND_PARAMS = _build_label_group(_MESSAGE_AND_PARAMS_TYPES)


def rewrite_text_clientbound(wrapper: PacketWrapper) -> None:
    """Rewrite Text from the v860 wire format to the v898 format.

    Args:
        wrapper: Packet wrapper for Text.
    """
    text_type = wrapper.read(BYTE)
    needs_translation = wrapper.read(BOOL)

    wrapper.write(BOOL, needs_translation)
    if text_type in _TEXT_MESSAGE_ONLY:
        wrapper.write(BYTE, TextPacketBodyType.MESSAGE_ONLY)
        for label in _TEXT_MESSAGE_ONLY[text_type]:
            wrapper.write(STRING, label)
        wrapper.write(BYTE, text_type)
        wrapper.passthrough(STRING)
    elif text_type in _TEXT_AUTHOR_AND_MESSAGE:
        wrapper.write(BYTE, TextPacketBodyType.AUTHOR_AND_MESSAGE)
        for label in _TEXT_AUTHOR_AND_MESSAGE[text_type]:
            wrapper.write(STRING, label)
        wrapper.write(BYTE, text_type)
        wrapper.passthrough(STRING)
        wrapper.passthrough(STRING)
    elif text_type in _TEXT_MESSAGE_AND_PARAMS:
        wrapper.write(BYTE, TextPacketBodyType.MESSAGE_AND_PARAMS)
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
    wrapper.write(OptionalType(STRING), filtered_message or None)


def rewrite_text_serverbound(wrapper: PacketWrapper) -> None:
    """Rewrite Text from the v898 wire format to the v860 format.

    Args:
        wrapper: Packet wrapper for Text.
    """
    needs_translation = wrapper.read(BOOL)
    kind = wrapper.read(BYTE)

    if kind == TextPacketBodyType.MESSAGE_ONLY:
        for _ in range(6):
            wrapper.read(STRING)
        text_type = wrapper.read(BYTE)
        wrapper.write(BYTE, text_type)
        wrapper.write(BOOL, needs_translation)
        wrapper.passthrough(STRING)
    elif kind == TextPacketBodyType.AUTHOR_AND_MESSAGE:
        for _ in range(3):
            wrapper.read(STRING)
        text_type = wrapper.read(BYTE)
        wrapper.write(BYTE, text_type)
        wrapper.write(BOOL, needs_translation)
        wrapper.passthrough(STRING)
        wrapper.passthrough(STRING)
    elif kind == TextPacketBodyType.MESSAGE_AND_PARAMS:
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
    wrapper.write(STRING, wrapper.read(OptionalType(STRING)) or "")
