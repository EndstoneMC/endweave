"""Helpers shared across Endweave.

See Also:
    com.viaversion.viaversion.util.ChatColorUtil
"""

from endstone import ColorFormat

__all__ = ["COLOR_CHAR", "translate_alternate_color_codes"]

COLOR_CHAR = "§"

_ALL_CODES: frozenset[str] = frozenset(
    code[1:]
    for code in (getattr(ColorFormat, name) for name in dir(ColorFormat))
    if isinstance(code, str) and code.startswith(COLOR_CHAR)
)


def translate_alternate_color_codes(text: str) -> str:
    """Turn a '&' in front of a colour or format code into the section sign.

    Only a character Endstone knows as a colour or format code is translated,
    and it is lowercased along the way. Anything else is left as it was typed.

    Args:
        text: Text to translate.

    Returns:
        The translated text.
    """
    characters = list(text)
    for index in range(len(characters) - 1):
        if characters[index] == "&" and characters[index + 1].lower() in _ALL_CODES:
            characters[index] = COLOR_CHAR
            characters[index + 1] = characters[index + 1].lower()
    return "".join(characters)
