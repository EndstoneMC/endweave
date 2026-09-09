"""Config merging against the packaged defaults, and the options read out of it."""

from __future__ import annotations

from pathlib import Path
from unittest.mock import MagicMock

import pytest
import tomlkit

from endweave.config import BlockedProtocolVersions, Config, ConfigSection, ConfigurationProvider, EndweaveConfig
from endweave.protocol.version import UNKNOWN, get_by_name, get_protocol

DEFAULTS = """\
# Whether the thing is on
enabled = true
count = 3
name = "default"

[section]
nested = 1

[servers]
"""


class StubConfig(Config):
    """A config with its defaults inline instead of packaged."""

    def default_config_text(self) -> str:
        return DEFAULTS


class ModifiableKeysConfig(StubConfig):
    def sections_with_modifiable_keys(self) -> tuple[str, ...]:
        return ("servers",)


@pytest.fixture
def config_file(tmp_path: Path) -> Path:
    return tmp_path / "config.toml"


@pytest.fixture
def config(config_file: Path, mock_logger: MagicMock) -> StubConfig:
    return StubConfig(config_file, mock_logger)


@pytest.fixture
def endweave_config(config_file: Path, mock_logger: MagicMock) -> EndweaveConfig:
    config = EndweaveConfig(config_file, mock_logger)
    config.reload()
    return config


def write(config_file: Path, text: str) -> None:
    config_file.write_text(text, encoding="utf-8")


class TestLoading:
    def test_writes_the_defaults_when_no_file_exists(self, config: StubConfig, config_file: Path) -> None:
        config.reload()

        assert config_file.read_text() == DEFAULTS
        assert config.get_bool("enabled", False) is True

    def test_keeps_a_value_the_user_has_changed(self, config: StubConfig, config_file: Path) -> None:
        write(config_file, 'enabled = false\ncount = 3\nname = "default"\n')
        config.reload()

        assert config.get_bool("enabled", True) is False
        assert config.get_int("count", 0) == 3

    def test_adds_options_missing_from_an_older_file(self, config: StubConfig, config_file: Path) -> None:
        write(config_file, "enabled = false\n")
        config.reload()

        assert config.get_int("count", 0) == 3
        assert config.get_string("name", "") == "default"

    def test_drops_options_the_defaults_no_longer_carry(self, config: StubConfig, config_file: Path) -> None:
        write(config_file, "enabled = true\nlong-gone = 42\n")
        config.reload()

        assert config.contains("long-gone") is False
        assert "long-gone" not in config_file.read_text()

    def test_merges_into_a_section(self, config: StubConfig, config_file: Path) -> None:
        write(config_file, "[section]\nnested = 9\ngone = 1\n")
        config.reload()

        section = config.section("section")
        assert section is not None
        assert section.get_int("nested", 0) == 9
        assert section.contains("gone") is False

    def test_restores_the_comments_of_the_defaults(self, config: StubConfig, config_file: Path) -> None:
        write(config_file, "enabled = false\n")
        config.reload()

        assert "# Whether the thing is on" in config_file.read_text()

    def test_keeps_user_added_keys_of_a_modifiable_section(self, config_file: Path, mock_logger: MagicMock) -> None:
        write(config_file, '[servers]\nlobby = "1.26.20"\n')
        config = ModifiableKeysConfig(config_file, mock_logger)
        config.reload()

        section = config.section("servers")
        assert section is not None
        assert section.get_string("lobby", "") == "1.26.20"

    def test_leaves_an_up_to_date_file_alone(
        self, config: StubConfig, config_file: Path, monkeypatch: pytest.MonkeyPatch
    ) -> None:
        config.reload()

        saved: list[Path] = []
        monkeypatch.setattr(StubConfig, "save_to", lambda self, location, values: saved.append(location))
        config.reload()

        assert saved == []

    def test_reports_the_path_of_a_malformed_file(
        self, config: StubConfig, config_file: Path, mock_logger: MagicMock
    ) -> None:
        write(config_file, "enabled = = true\n")

        with pytest.raises(tomlkit.exceptions.ParseError):
            config.reload()
        assert str(config_file) in mock_logger.error.call_args.args[0]

    def test_exposes_the_file_as_it_was_before_merging(self, config: StubConfig, config_file: Path) -> None:
        write(config_file, "enabled = false\n")
        config.reload()

        original = config.original_root_section
        assert original is not None
        assert original.contains("count") is False

    def test_has_no_original_section_for_a_file_it_created(self, config: StubConfig) -> None:
        config.reload()

        assert config.original_root_section is None

    def test_save_writes_the_current_values(self, config: StubConfig, config_file: Path) -> None:
        config.reload()
        config.set("count", 7)
        config.save()

        assert tomlkit.parse(config_file.read_text())["count"] == 7


class TestSectionGetters:
    def test_falls_back_when_the_type_is_wrong(self, config: StubConfig) -> None:
        config.reload()

        assert config.get_bool("count", False) is False
        assert config.get_int("name", -1) == -1
        assert config.get_string("count", "fallback") == "fallback"
        assert config.get_float("name", 1.5) == 1.5

    def test_reads_an_int_as_a_float(self, config: StubConfig) -> None:
        config.reload()

        assert config.get_float("count", 0.0) == 3.0

    def test_follows_a_dotted_path(self, config: StubConfig) -> None:
        config.reload()

        assert config.get("section.nested") == 1
        assert config.get("section.missing", "fallback") == "fallback"
        assert config.get("missing.nested", "fallback") == "fallback"

    def test_section_of_a_non_section_is_none(self, config: StubConfig) -> None:
        config.reload()

        assert config.section("count") is None

    def test_section_path_is_dotted_from_the_root(self, config: StubConfig) -> None:
        config.reload()

        section = config.section("section")
        assert section is not None
        assert section.path == "section"

    def test_list_keeps_the_entries_of_the_right_type(self, config: StubConfig, mock_logger: MagicMock) -> None:
        config.reload()
        config.set("protocols", [924, True, "944"])

        assert config.get_list_safe("protocols", int, "bad: '%s'") == [924]
        assert [call.args[0] for call in mock_logger.warning.call_args_list] == ["bad: 'True'", "bad: '944'"]

    def test_list_stays_quiet_without_a_message(self, config: StubConfig, mock_logger: MagicMock) -> None:
        config.reload()
        config.set("protocols", ["944"])

        assert config.get_list_safe("protocols", int) == []
        mock_logger.warning.assert_not_called()

    def test_list_of_a_missing_key_is_empty(self, config: StubConfig) -> None:
        config.reload()

        assert config.get_list_safe("missing", str) == []


class TestConfigurationProvider:
    def test_reloads_every_registered_config(self, tmp_path: Path, mock_logger: MagicMock) -> None:
        first = StubConfig(tmp_path / "first.toml", mock_logger)
        second = StubConfig(tmp_path / "second.toml", mock_logger)
        provider = ConfigurationProvider()
        provider.register(first)
        provider.register(second)

        provider.reload_configs()

        assert provider.configs == (first, second)
        assert (tmp_path / "first.toml").exists()
        assert (tmp_path / "second.toml").exists()


class TestEndweaveOptions:
    def test_reads_the_packaged_defaults(self, endweave_config: EndweaveConfig) -> None:
        assert endweave_config.check_for_updates is True
        assert endweave_config.blocked_disconnect_message == "You are using an unsupported Minecraft version!"
        assert endweave_config.log_blocked_joins is False
        assert endweave_config.log_entity_data_errors is True
        assert endweave_config.log_other_conversion_warnings is False
        assert endweave_config.max_error_length == 1500

    def test_setting_check_for_updates_writes_the_option(self, endweave_config: EndweaveConfig) -> None:
        endweave_config.check_for_updates = False

        assert endweave_config.check_for_updates is False
        assert endweave_config.get_bool("check-for-updates", True) is False

    def test_reload_picks_up_an_edit_on_disk(self, config_file: Path, mock_logger: MagicMock) -> None:
        config = EndweaveConfig(config_file, mock_logger)
        config.reload()
        assert config.check_for_updates is True

        write(config_file, "check-for-updates = false\n")
        config.reload()

        assert config.check_for_updates is False

    def test_reads_the_logging_section(self, config_file: Path, mock_logger: MagicMock) -> None:
        write(config_file, "[logging]\nmax-error-length = 20\nlog-blocked-joins = true\n")
        config = EndweaveConfig(config_file, mock_logger)
        config.reload()

        assert config.max_error_length == 20
        assert config.log_blocked_joins is True


class TestBlockedProtocolVersions:
    def load(self, config_file: Path, logger: MagicMock, text: str) -> BlockedProtocolVersions:
        write(config_file, text)
        config = EndweaveConfig(config_file, logger)
        config.reload()
        return config.blocked_protocol_versions

    def test_nothing_is_blocked_by_default(self, endweave_config: EndweaveConfig) -> None:
        blocked = endweave_config.blocked_protocol_versions

        assert blocked.single_blocked_versions == frozenset()
        assert blocked.blocks_below is UNKNOWN
        assert blocked.blocks_above is UNKNOWN
        assert get_protocol(975) not in blocked

    def test_blocks_protocol_numbers(self, config_file: Path, mock_logger: MagicMock) -> None:
        blocked = self.load(config_file, mock_logger, "block-protocols = [975]\n")

        assert get_protocol(975) in blocked
        assert get_protocol(944) not in blocked

    def test_blocks_a_version_name(self, config_file: Path, mock_logger: MagicMock) -> None:
        blocked = self.load(config_file, mock_logger, 'block-versions = ["1.26.20"]\n')

        assert blocked.single_blocked_versions == frozenset({get_protocol(975)})
        assert get_protocol(975) in blocked

    def test_blocks_a_version_covered_by_a_range_name(self, config_file: Path, mock_logger: MagicMock) -> None:
        blocked = self.load(config_file, mock_logger, 'block-versions = ["1.26.2"]\n')

        assert get_protocol(924) in blocked

    def test_bounds_block_everything_beyond_them(self, config_file: Path, mock_logger: MagicMock) -> None:
        blocked = self.load(config_file, mock_logger, 'block-versions = ["<1.26.0", ">1.26.30"]\n')

        assert blocked.blocks_below == get_protocol(924)
        assert blocked.blocks_above == get_protocol(1001)
        assert get_protocol(898) in blocked
        assert get_protocol(2168) in blocked
        assert get_protocol(944) not in blocked

    def test_warns_about_an_unknown_version(self, config_file: Path, mock_logger: MagicMock) -> None:
        blocked = self.load(config_file, mock_logger, 'block-versions = ["1.99.0"]\n')

        assert blocked.single_blocked_versions == frozenset()
        mock_logger.warning.assert_called_once_with("Unknown protocol version in block-versions: 1.99.0")

    def test_warns_when_a_bound_is_set_twice(self, config_file: Path, mock_logger: MagicMock) -> None:
        blocked = self.load(config_file, mock_logger, 'block-versions = ["<1.26.0", "<1.26.20"]\n')

        assert blocked.blocks_below == get_protocol(975)
        assert "overridden by" in mock_logger.warning.call_args.args[0]

    def test_warns_about_a_version_blocked_twice(self, config_file: Path, mock_logger: MagicMock) -> None:
        blocked = self.load(config_file, mock_logger, 'block-protocols = [975]\nblock-versions = ["1.26.20"]\n')

        assert blocked.single_blocked_versions == frozenset({get_protocol(975)})
        mock_logger.warning.assert_called_once_with(f"Duplicated blocked protocol version {get_protocol(975)}")

    def test_drops_a_version_a_bound_already_covers(self, config_file: Path, mock_logger: MagicMock) -> None:
        blocked = self.load(config_file, mock_logger, 'block-versions = ["1.21.130", "<1.26.0"]\n')

        assert blocked.single_blocked_versions == frozenset()
        assert get_protocol(898) in blocked
        assert "already covered by upper or lower bound" in mock_logger.warning.call_args.args[0]

    def test_ignores_an_empty_entry(self, config_file: Path, mock_logger: MagicMock) -> None:
        blocked = self.load(config_file, mock_logger, 'block-versions = [""]\n')

        assert blocked.single_blocked_versions == frozenset()
        mock_logger.warning.assert_not_called()

    def test_unknown_bounds_block_nothing(self) -> None:
        blocked = BlockedProtocolVersions()

        assert get_by_name("1.26.20") not in blocked


class TestSectionConstruction:
    def test_a_bare_section_reads_its_own_values(self, config: StubConfig) -> None:
        section = ConfigSection(config, "logging", {"max-error-length": 5})

        assert section.get_int("max-error-length", 0) == 5
        assert section.root is config
