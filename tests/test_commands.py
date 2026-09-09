"""The list subcommand: grouping, ordering, and the metadata a dispatcher reads."""

from __future__ import annotations

from unittest.mock import MagicMock

import pytest
from endstone.command import CommandSender

from endweave._version import __version__
from endweave.commands import CommandHandler, ListSubCommand, SubCommand
from endweave.plugin import EndweavePlugin


@pytest.fixture
def mock_sender() -> MagicMock:
    """Command sender double whose .send_message calls are the assertion target."""
    sender = MagicMock()
    sender.server.online_players = []
    return sender


def add_players(sender: MagicMock, **players: str) -> None:
    for name, game_version in players.items():
        player = MagicMock()
        player.name = name
        player.game_version = game_version
        sender.server.online_players.append(player)


def sent(sender: MagicMock) -> list[str]:
    return [call.args[0] for call in sender.send_message.call_args_list]


def test_no_players_reports_an_empty_server(mock_sender: MagicMock) -> None:
    assert ListSubCommand().execute(mock_sender, []) is True
    assert sent(mock_sender) == ["§cNo players found!"]


def test_players_on_one_version_are_grouped_and_counted(mock_sender: MagicMock) -> None:
    add_players(mock_sender, bob="1.26.30", alice="1.26.32")
    ListSubCommand().execute(mock_sender, [])
    assert sent(mock_sender) == ["§8[§61.26.30-1.26.32§8] (§72§8): §balice, bob"]


def test_versions_are_listed_oldest_protocol_first(mock_sender: MagicMock) -> None:
    add_players(mock_sender, carol="1.26.50", alice="1.21.124", bob="1.26.0")
    ListSubCommand().execute(mock_sender, [])
    assert sent(mock_sender) == [
        "§8[§61.21.124§8] (§71§8): §balice",
        "§8[§61.26.0-1.26.3§8] (§71§8): §bbob",
        "§8[§61.26.5x§8] (§71§8): §bcarol",
    ]


def test_an_unregistered_game_version_falls_back_to_unknown(mock_sender: MagicMock) -> None:
    add_players(mock_sender, alice="1.99.0", bob="1.26.0")
    ListSubCommand().execute(mock_sender, [])
    assert sent(mock_sender) == [
        "§8[§6UNKNOWN§8] (§71§8): §balice",
        "§8[§61.26.0-1.26.3§8] (§71§8): §bbob",
    ]


def test_the_permission_defaults_off_the_name() -> None:
    assert ListSubCommand().permission == "endweave.command.list"


def test_the_permission_is_declared_as_a_child_of_endweave_admin() -> None:
    children = EndweavePlugin.permissions["endweave.admin"]["children"]
    assert children == {ListSubCommand().permission: True}


class NamedSubCommand(SubCommand):
    def __init__(self, name: str) -> None:
        self._name = name

    @property
    def name(self) -> str:
        return self._name

    @property
    def description(self) -> str:
        return "Does nothing at all."

    def execute(self, sender: CommandSender, args: list[str]) -> bool:
        return False


@pytest.fixture
def handler() -> CommandHandler:
    """A handler with the default subcommands registered, as on_enable builds it."""
    return CommandHandler()


def test_a_known_word_routes_to_its_subcommand(handler: CommandHandler, mock_sender: MagicMock) -> None:
    assert handler.on_command(mock_sender, MagicMock(), ["list"]) is True
    assert sent(mock_sender) == ["§cNo players found!"]


def test_no_arguments_shows_the_help_listing(handler: CommandHandler, mock_sender: MagicMock) -> None:
    assert handler.on_command(mock_sender, MagicMock(), []) is False
    assert sent(mock_sender) == [
        f"§aEndweave §c{__version__}",
        "§6Commands:",
        "§2/endweave list §7- §6Shows lists of the versions from logged in players.",
    ]


def test_an_unknown_word_says_so_then_shows_the_help(handler: CommandHandler, mock_sender: MagicMock) -> None:
    assert handler.on_command(mock_sender, MagicMock(), ["nope"]) is False
    assert sent(mock_sender)[0] == "§cThis command does not exist."
    assert "§6Commands:" in sent(mock_sender)


def test_a_sender_with_no_permission_is_turned_away(handler: CommandHandler, mock_sender: MagicMock) -> None:
    mock_sender.has_permission.return_value = False
    assert handler.on_command(mock_sender, MagicMock(), ["list"]) is False
    assert sent(mock_sender) == ["§cYou are not allowed to use this command!"]


def test_the_admin_permission_alone_opens_every_subcommand(handler: CommandHandler, mock_sender: MagicMock) -> None:
    mock_sender.has_permission.side_effect = lambda name: name == "endweave.admin"
    assert handler.on_command(mock_sender, MagicMock(), ["list"]) is True
    assert sent(mock_sender) == ["§cNo players found!"]


def test_a_subcommand_returning_false_gets_a_usage_line(mock_sender: MagicMock) -> None:
    handler = CommandHandler()
    handler.register_subcommand(NamedSubCommand("noop"))
    assert handler.on_command(mock_sender, MagicMock(), ["noop"]) is False
    assert sent(mock_sender) == ["Usage: /endweave noop"]


def test_a_malformed_subcommand_name_is_rejected(handler: CommandHandler) -> None:
    with pytest.raises(ValueError, match="not a valid subcommand name"):
        handler.register_subcommand(NamedSubCommand("no"))


def test_a_duplicate_subcommand_name_is_rejected(handler: CommandHandler) -> None:
    with pytest.raises(ValueError, match="does already exist"):
        handler.register_subcommand(NamedSubCommand("list"))
