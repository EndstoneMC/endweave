"""Translation of the '&' colour codes of a message into the section sign."""

from endweave.util import translate_alternate_color_codes


class TestTranslateAlternateColorCodes:
    def test_translates_a_colour_code(self) -> None:
        assert translate_alternate_color_codes("&cUnsupported") == "§cUnsupported"

    def test_translates_every_code_in_the_message(self) -> None:
        assert translate_alternate_color_codes("&lBold &r and &gminecoin") == "§lBold §r and §gminecoin"

    def test_lowercases_the_code(self) -> None:
        assert translate_alternate_color_codes("&CUnsupported") == "§cUnsupported"

    def test_leaves_a_character_that_is_not_a_code_alone(self) -> None:
        assert translate_alternate_color_codes("Tom & Jerry, black &white, R&z") == "Tom & Jerry, black &white, R&z"

    def test_leaves_a_trailing_ampersand_alone(self) -> None:
        assert translate_alternate_color_codes("Rock &") == "Rock &"

    def test_leaves_a_section_sign_alone(self) -> None:
        assert translate_alternate_color_codes("§cUnsupported") == "§cUnsupported"

    def test_leaves_a_message_without_codes_alone(self) -> None:
        assert translate_alternate_color_codes("") == ""
        assert translate_alternate_color_codes("Unsupported") == "Unsupported"
