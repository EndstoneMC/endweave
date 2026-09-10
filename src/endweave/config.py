"""Configuration loading for Endweave.

Ports ViaVersion's config stack: the merge-on-load mechanism of its ``Config``
and ``ConfigSection``, the ``ConfigurationProvider`` registry, and the options
of ``ViaVersionConfig`` that are not tied to Java Edition.

Endstone plugins ship TOML, so tomlkit stands in for snakeyaml, and the
packaged ``config.toml`` doubles as ViaVersion's ``CommentStore``: it is parsed
with its comments intact, the values a user has already set are written into
it, and that document is what gets saved. Comments therefore stay in step with
the defaults without a store of their own.

Two pieces of ViaVersion have no counterpart here. ``getUnsupportedOptions``
strips Bukkit or Velocity only options from a config shared across platforms,
and Endstone is the only platform Endweave runs on. The sixty odd gameplay
options behind them, shield blocking, hologram offsets, piston animation and
the rest, describe Java Edition fixes Bedrock never needed. The packet and
packet size limiters are left out too, having nothing to enforce them.

See Also:
    com.viaversion.viaversion.api.configuration.Config
    com.viaversion.viaversion.api.configuration.ConfigurationProvider
    com.viaversion.viaversion.api.configuration.ViaVersionConfig
    com.viaversion.viaversion.api.protocol.version.BlockedProtocolVersions
    com.viaversion.viaversion.configuration.AbstractViaConfig
    com.viaversion.viaversion.configuration.ConfigurationProviderImpl
    com.viaversion.viaversion.util.Config
    com.viaversion.viaversion.util.ConfigSection
"""

from __future__ import annotations

import importlib.resources
from abc import ABC, abstractmethod
from collections.abc import Collection, MutableMapping
from dataclasses import dataclass
from pathlib import Path
from typing import Any, TypeVar

import tomlkit
from endstone import Logger
from tomlkit import TOMLDocument

from .protocol.version import UNKNOWN, ProtocolVersion, get_by_name, get_protocol

__all__ = [
    "BlockedProtocolVersions",
    "Config",
    "ConfigSection",
    "ConfigurationProvider",
    "EndweaveConfig",
]

T = TypeVar("T")


class ConfigSection:
    """A table of config values, read through typed getters with defaults.

    Attributes:
        path: Dotted path of this section from the root, empty at the root.

    See Also:
        com.viaversion.viaversion.util.ConfigSection
    """

    def __init__(self, root: Config, path: str, values: MutableMapping[str, Any] | None = None) -> None:
        self._root = root
        self.path = path
        self._values: MutableMapping[str, Any] = {} if values is None else values

    @property
    def root(self) -> Config:
        """The config this section belongs to."""
        return self._root

    @property
    def logger(self) -> Logger:
        """The logger of the config this section belongs to."""
        return self.root.logger

    @property
    def values(self) -> MutableMapping[str, Any]:
        """The mapping backing this section."""
        return self._values

    def set(self, key: str, value: Any) -> None:
        """Set a key in this section.

        Args:
            key: Key to set.
            value: Value to set it to.
        """
        self._values[key] = value

    def get(self, key: str, default: Any = None) -> Any:
        """Read a value, following a dotted key through nested sections.

        Args:
            key: Key to read, e.g. "logging.max-error-length".
            default: Returned when the key is missing.

        Returns:
            The value, or ``default``.
        """
        *parents, value_key = key.split(".")
        section: ConfigSection = self
        for parent in parents:
            child = section.section(parent)
            if child is None:
                return default
            section = child

        value = section.values.get(value_key)
        return default if value is None else value

    def contains(self, key: str) -> bool:
        """Whether a value is set at this key.

        Args:
            key: Key to look for, dotted paths included.

        Returns:
            Whether the key holds a value.
        """
        return self.get(key) is not None

    def get_bool(self, key: str, default: bool) -> bool:
        """Read a boolean, or ``default`` if the key is missing or another type."""
        value = self.get(key)
        return bool(value) if isinstance(value, bool) else default

    def get_string(self, key: str, default: str) -> str:
        """Read a string, or ``default`` if the key is missing or another type."""
        value = self.get(key)
        return str(value) if isinstance(value, str) else default

    def get_int(self, key: str, default: int) -> int:
        """Read an integer, or ``default`` if the key is missing or not a number."""
        value = self.get(key)
        return int(value) if isinstance(value, (int, float)) and not isinstance(value, bool) else default

    def get_float(self, key: str, default: float) -> float:
        """Read a float, or ``default`` if the key is missing or not a number."""
        value = self.get(key)
        return float(value) if isinstance(value, (int, float)) and not isinstance(value, bool) else default

    def get_list_safe(self, key: str, item_type: type[T], invalid_value_message: str | None = None) -> list[T]:
        """Read a list, dropping the entries that are of the wrong type.

        A boolean counts as an integer in Python, so one is only kept when
        ``bool`` is the type asked for.

        Args:
            key: Key holding the list.
            item_type: Type every entry must have.
            invalid_value_message: Warning logged per dropped entry, with a
                single ``%s`` for the entry itself. None to drop them quietly.
                A key holding something other than a list is warned about by
                key name instead.

        Returns:
            The entries of the requested type, or an empty list if the key
            holds no list.
        """
        value = self.get(key)
        if not isinstance(value, list):
            if value is not None and invalid_value_message is not None:
                self.logger.warning(f"Config option {key} must be a list, ignoring it")
            return []

        values: list[T] = []
        for item in value:
            if isinstance(item, item_type) and (item_type is bool or not isinstance(item, bool)):
                values.append(item)
            elif invalid_value_message is not None:
                self.logger.warning(invalid_value_message % (item,))
        return values

    def section(self, key: str) -> ConfigSection | None:
        """Read a nested section.

        Args:
            key: Key holding the section, dotted paths included.

        Returns:
            The section, or None if the key holds something else.
        """
        value = self.get(key)
        if not isinstance(value, dict):
            return None
        return ConfigSection(self.root, key if not self.path else f"{self.path}.{key}", value)


class Config(ConfigSection, ABC):
    """A config file merged against packaged defaults every time it is loaded.

    Loading starts from the packaged defaults and writes the values already set
    in the file over them, so options added in a release show up in a config
    written by an older one, and options no longer packaged fall away. The file
    is rewritten whenever that merge changed anything.

    Constructing a config does not load it. Call ``reload`` first.

    See Also:
        com.viaversion.viaversion.util.Config
    """

    def __init__(self, config_file: Path, logger: Logger) -> None:
        """Create a config.

        Args:
            config_file: Where the config is loaded from and saved to.
            logger: Logger to report config problems through.
        """
        super().__init__(self, "")
        self._config_file = config_file
        self._logger = logger
        self._original_root: ConfigSection | None = None

    @property
    def logger(self) -> Logger:
        """The logger config problems are reported through."""
        return self._logger

    @property
    def config_file(self) -> Path:
        """Where the config is loaded from and saved to."""
        return self._config_file

    @property
    def original_root_section(self) -> ConfigSection | None:
        """The config as it was on disk before merging, or None if there was none."""
        return self._original_root

    @abstractmethod
    def default_config_text(self) -> str:
        """Return the packaged default config, comments and all."""

    def sections_with_modifiable_keys(self) -> Collection[str]:
        """Sections users may add their own keys to.

        Keys of these sections survive the merge even though the defaults do
        not list them. Everywhere else a key absent from the defaults is
        dropped.

        Returns:
            The keys of those sections.
        """
        return ()

    def load_config(self, location: Path) -> MutableMapping[str, Any]:
        """Merge the config at a path onto the packaged defaults.

        Saves the result back when it differs from what was read, which also
        writes the file when it does not exist yet.

        Args:
            location: Config file to read.

        Returns:
            The merged values.

        Raises:
            tomlkit.exceptions.ParseError: If the existing config is malformed.
        """
        self._original_root = None

        existing: TOMLDocument | None = None
        if location.exists():
            try:
                existing = tomlkit.parse(location.read_text(encoding="utf-8"))
            except Exception:
                self._logger.error(f"Failed to load {location}, make sure your input is valid")
                raise

        merged = tomlkit.parse(self.default_config_text())
        if existing is not None:
            self._merge(None, existing, merged)
            self._original_root = ConfigSection(self, "", existing)

        if existing is None or merged.unwrap() != existing.unwrap():
            self.save_to(location, merged)

        return merged

    def _merge(
        self,
        section_key: str | None,
        loaded: MutableMapping[str, Any],
        merged: MutableMapping[str, Any],
    ) -> None:
        for key, value in loaded.items():
            merged_value = merged.get(key)
            if isinstance(value, dict) and isinstance(merged_value, dict):
                self._merge(key, value, merged_value)
            elif key in merged and isinstance(value, dict) != isinstance(merged_value, dict):
                name = key if section_key is None else f"{section_key}.{key}"
                expected = "a table" if isinstance(merged_value, dict) else "a single value"
                self.logger.warning(f"Config option {name} must be {expected}, keeping the default")
            elif section_key in self.sections_with_modifiable_keys() or key in merged:
                merged[key] = value

    def save_to(self, location: Path, values: MutableMapping[str, Any]) -> None:
        """Write values to a path, creating the directory if it is missing.

        Args:
            location: File to write.
            values: Values to write.
        """
        location.parent.mkdir(parents=True, exist_ok=True)
        location.write_text(tomlkit.dumps(values), encoding="utf-8")

    def save(self) -> None:
        """Write the current values back to the config file."""
        self.save_to(self._config_file, self._values)

    def reload(self) -> None:
        """Load the config file, merging it onto the packaged defaults."""
        self._values = self.load_config(self._config_file)


class ConfigurationProvider:
    """The configs that are reloaded together.

    See Also:
        com.viaversion.viaversion.api.configuration.ConfigurationProvider
        com.viaversion.viaversion.configuration.ConfigurationProviderImpl
    """

    def __init__(self) -> None:
        self._configs: list[Config] = []

    def register(self, config: Config) -> None:
        """Register a config to be reloaded along with the others.

        Args:
            config: The config to register.
        """
        self._configs.append(config)

    @property
    def configs(self) -> tuple[Config, ...]:
        """The registered configs."""
        return tuple(self._configs)

    def reload_configs(self) -> None:
        """Reload every registered config."""
        for config in self._configs:
            config.reload()


@dataclass(frozen=True)
class BlockedProtocolVersions:
    """The protocol versions a server refuses, as bounds plus single versions.

    Attributes:
        single_blocked_versions: Versions blocked one at a time, between the bounds.
        blocks_below: Versions older than this are blocked, UNKNOWN when unset.
        blocks_above: Versions newer than this are blocked, UNKNOWN when unset.

    See Also:
        com.viaversion.viaversion.api.protocol.version.BlockedProtocolVersions
        com.viaversion.viaversion.protocol.BlockedProtocolVersionsImpl
    """

    single_blocked_versions: frozenset[ProtocolVersion] = frozenset()
    blocks_below: ProtocolVersion = UNKNOWN
    blocks_above: ProtocolVersion = UNKNOWN

    def __contains__(self, protocol_version: ProtocolVersion) -> bool:
        return (
            (self.blocks_below.known and protocol_version < self.blocks_below)
            or (self.blocks_above.known and protocol_version > self.blocks_above)
            or protocol_version in self.single_blocked_versions
        )


class EndweaveConfig(Config):
    """Endweave's config file and the options read out of it.

    Every option is read once per ``reload`` and served from the field it was
    read into, so a property is only meaningful after the config has been
    loaded at least once.

    See Also:
        com.viaversion.viaversion.api.configuration.ViaVersionConfig
        com.viaversion.viaversion.configuration.AbstractViaConfig
    """

    _check_for_updates: bool
    _blocked_protocol_versions: BlockedProtocolVersions
    _blocked_disconnect_message: str
    _reload_disconnect_message: str
    _log_blocked_joins: bool
    _log_entity_data_errors: bool
    _log_other_conversion_warnings: bool
    _max_error_length: int

    def default_config_text(self) -> str:
        return (importlib.resources.files("endweave") / "config.toml").read_text(encoding="utf-8")

    def reload(self) -> None:
        super().reload()
        if self.update_config():
            self.save()
        self._load_fields()

    def update_config(self) -> bool:
        """Bring an existing config up to date where merging cannot.

        Renamed options and changed defaults are handled here, keyed off
        ``original_root_section`` and the ``config-version`` it carries. There
        is nothing to migrate yet.

        Returns:
            Whether the config should be saved afterwards.
        """
        return False

    def _load_fields(self) -> None:
        self._check_for_updates = self.get_bool("check-for-updates", True)
        self._blocked_protocol_versions = self._load_blocked_protocol_versions()
        self._blocked_disconnect_message = self.get_string(
            "block-disconnect-msg", "You are using an unsupported Minecraft version!"
        )
        self._reload_disconnect_message = self.get_string("reload-disconnect-msg", "Server reload, please rejoin!")

        logging_section = self.section("logging") or ConfigSection(self, "logging")
        self._log_blocked_joins = logging_section.get_bool("log-blocked-joins", False)
        self._log_entity_data_errors = logging_section.get_bool("log-entity-data-errors", True)
        self._log_other_conversion_warnings = logging_section.get_bool("log-other-conversion-warnings", False)
        self._max_error_length = logging_section.get_int("max-error-length", 1500)

    def _load_blocked_protocol_versions(self) -> BlockedProtocolVersions:
        block_protocols = self.get_list_safe(
            "block-protocols", int, "Invalid blocked version protocol found in config: '%s'"
        )
        block_versions = self.get_list_safe("block-versions", str, "Invalid blocked version found in config: '%s'")

        blocked_protocols = {get_protocol(protocol) for protocol in block_protocols}
        lower_bound = UNKNOWN
        upper_bound = UNKNOWN
        for entry in block_versions:
            if not entry:
                continue

            bound = entry[0] if entry[0] in "<>" else ""
            name = entry[1:] if bound else entry
            protocol_version = get_by_name(name)
            if protocol_version is None:
                self.logger.warning(f"Unknown protocol version in block-versions: {name}")
                continue

            if bound == "<":
                if lower_bound.known:
                    self.logger.warning(f"Already set lower bound {lower_bound} overridden by {protocol_version.name}")
                lower_bound = protocol_version
            elif bound == ">":
                if upper_bound.known:
                    self.logger.warning(f"Already set upper bound {upper_bound} overridden by {protocol_version.name}")
                upper_bound = protocol_version
            else:
                if protocol_version in blocked_protocols:
                    self.logger.warning(f"Duplicated blocked protocol version {protocol_version}")
                blocked_protocols.add(protocol_version)

        covered = {
            version
            for version in blocked_protocols
            if (lower_bound.known and version < lower_bound) or (upper_bound.known and version > upper_bound)
        }
        for version in covered:
            self.logger.warning(f"Blocked protocol version {version} already covered by upper or lower bound")

        return BlockedProtocolVersions(frozenset(blocked_protocols - covered), lower_bound, upper_bound)

    @property
    def check_for_updates(self) -> bool:
        """Whether to check for a newer release on startup and on join."""
        return self._check_for_updates

    @check_for_updates.setter
    def check_for_updates(self, check_for_updates: bool) -> None:
        self._check_for_updates = check_for_updates
        self.set("check-for-updates", check_for_updates)

    @property
    def blocked_protocol_versions(self) -> BlockedProtocolVersions:
        """The protocol versions that are refused."""
        return self._blocked_protocol_versions

    @property
    def blocked_disconnect_message(self) -> str:
        """Message shown to a player kicked for speaking a blocked version."""
        return self._blocked_disconnect_message

    @property
    def reload_disconnect_message(self) -> str:
        """Message shown to the players kicked when the plugin is reloaded."""
        return self._reload_disconnect_message

    @property
    def log_blocked_joins(self) -> bool:
        """Whether kicks caused by a blocked version are logged."""
        return self._log_blocked_joins

    @property
    def log_entity_data_errors(self) -> bool:
        """Whether errors while converting entity data are logged."""
        return self._log_entity_data_errors

    @property
    def log_other_conversion_warnings(self) -> bool:
        """Whether other conversion problems are warned about."""
        return self._log_other_conversion_warnings

    @property
    def max_error_length(self) -> int:
        """Longest error message written to the console."""
        return self._max_error_length
