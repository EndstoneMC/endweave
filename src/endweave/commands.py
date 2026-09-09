"""Subcommands of the Endweave command.

See Also:
    com.viaversion.viaversion.api.command.ViaSubCommand
    com.viaversion.viaversion.commands.ViaCommandHandler
    com.viaversion.viaversion.commands.defaultsubs.DebugSubCmd
    com.viaversion.viaversion.commands.defaultsubs.ListSubCmd
"""

from __future__ import annotations

import re
from abc import ABC, abstractmethod
from collections import defaultdict

from endstone.command import Command, CommandExecutor, CommandSender

from ._version import __version__
from .debug import DebugHandler
from .protocol.version import UNKNOWN, ProtocolVersion, get_by_name

__all__ = ["CommandHandler", "DebugSubCommand", "ListSubCommand", "SubCommand"]


class SubCommand(ABC):
    """One word of the Endweave command, with the metadata to dispatch it."""

    @property
    @abstractmethod
    def name(self) -> str:
        """The word that selects this subcommand."""

    @property
    @abstractmethod
    def description(self) -> str:
        """One line shown against this subcommand in the help listing."""

    @property
    def permission(self) -> str | None:
        """The permission required to run this, or None to allow everyone."""
        return f"endweave.command.{self.name}"

    @abstractmethod
    def execute(self, sender: CommandSender, args: list[str]) -> bool:
        """Run the subcommand.

        Args:
            sender: Whoever ran the command.
            args: Arguments following the subcommand name.

        Returns:
            Whether the subcommand handled the arguments it was given.
        """


class ListSubCommand(SubCommand):
    @property
    def name(self) -> str:
        return "list"

    @property
    def description(self) -> str:
        return "Shows lists of the versions from logged in players."

    def execute(self, sender: CommandSender, args: list[str]) -> bool:
        players_by_version: defaultdict[ProtocolVersion, set[str]] = defaultdict(set)
        for player in sender.server.online_players:
            players_by_version[get_by_name(player.game_version) or UNKNOWN].add(player.name)

        if not players_by_version:
            sender.send_message("§cNo players found!")
            return True

        for version in sorted(players_by_version):
            names = players_by_version[version]
            sender.send_message(f"§8[§6{version.name}§8] (§7{len(names)}§8): §b{', '.join(sorted(names))}")
        return True


class DebugSubCommand(SubCommand):
    """Toggles the debug modes and edits the packet type filter."""

    def __init__(self, debug_handler: DebugHandler) -> None:
        self._debug_handler = debug_handler

    @property
    def name(self) -> str:
        return "debug"

    @property
    def description(self) -> str:
        return "Toggle various debug modes."

    def execute(self, sender: CommandSender, args: list[str]) -> bool:
        debug = self._debug_handler
        if not args:
            debug.enabled = not debug.enabled
            sender.send_message(f"§6Debug mode is now {'§aenabled' if debug.enabled else '§cdisabled'}")
            return True

        action = args[0].lower()
        if len(args) == 1:
            if action == "clear":
                debug.clear_packet_types_to_log()
                sender.send_message("§6Cleared packet types to log")
                return True
            if action == "pre":
                debug.log_pre_packet_transform = not debug.log_pre_packet_transform
                state = "§aenabled" if debug.log_pre_packet_transform else "§cdisabled"
                sender.send_message(f"§6Pre transform packet logging is now {state}")
                return True
            if action == "post":
                debug.log_post_packet_transform = not debug.log_post_packet_transform
                state = "§aenabled" if debug.log_post_packet_transform else "§cdisabled"
                sender.send_message(f"§6Post transform packet logging is now {state}")
                return True
        elif len(args) == 2:
            if action == "add":
                debug.add_packet_type_name_to_log(args[1].upper())
                sender.send_message(f"§6Added packet type {args[1]} to debug logging")
                return True
            if action == "remove":
                debug.remove_packet_type_name_to_log(args[1].upper())
                sender.send_message(f"§6Removed packet type {args[1]} from debug logging")
                return True
        return False


class CommandHandler(CommandExecutor):
    def __init__(self, debug_handler: DebugHandler) -> None:
        super().__init__()
        self._subcommands: dict[str, SubCommand] = {}
        self.register_subcommand(ListSubCommand())
        self.register_subcommand(DebugSubCommand(debug_handler))

    def register_subcommand(self, subcommand: SubCommand) -> None:
        """Add a subcommand to the routing table.

        Args:
            subcommand: The subcommand to route to.

        Raises:
            ValueError: If the name is malformed or already taken.
        """
        name = subcommand.name.lower()
        if re.match(r"^[a-z0-9_-]{3,15}$", name) is None:
            raise ValueError(f"{subcommand.name} is not a valid subcommand name.")
        if name in self._subcommands:
            raise ValueError(f"SubCommand {subcommand.name} does already exist!")
        self._subcommands[name] = subcommand

    def on_command(self, sender: CommandSender, command: Command, args: list[str]) -> bool:
        if not any(self._is_allowed(sender, subcommand) for subcommand in self._subcommands.values()):
            sender.send_message("§cYou are not allowed to use this command!")
            return False

        if not args:
            self._show_help(sender)
            return False

        subcommand = self._subcommands.get(args[0].lower())
        if subcommand is None:
            sender.send_message("§cThis command does not exist.")
            self._show_help(sender)
            return False

        if not self._is_allowed(sender, subcommand):
            sender.send_message("§cYou are not allowed to use this command!")
            return False

        return subcommand.execute(sender, args[1:])

    def _is_allowed(self, sender: CommandSender, subcommand: SubCommand) -> bool:
        return (
            subcommand.permission is None
            or sender.has_permission("endweave.admin")
            or sender.has_permission(subcommand.permission)
        )

    def _show_help(self, sender: CommandSender) -> None:
        allowed = [subcommand for subcommand in self._subcommands.values() if self._is_allowed(sender, subcommand)]
        if not allowed:
            sender.send_message("§cYou are not allowed to use these commands!")
            return

        sender.send_message(f"§aEndweave §c{__version__}")
        sender.send_message("§6Commands:")
        for subcommand in allowed:
            sender.send_message(f"§2/endweave {subcommand.name} §7- §6{subcommand.description}")
