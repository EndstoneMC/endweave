"""Update notification for Endweave.

Checks the newest published release on startup and on player join, and reports
back through the console or a chat message. Notification only, nothing is
downloaded.

ViaVersion polls its own update server for a plain ``{"name": ...}`` document;
Endweave has no such service and reads the tag of the latest GitHub release
instead. Version comparison is PEP 440 rather than ViaVersion's own semver
type, so a hatch-vcs ``0.4.4.dev1`` build is recognised where a hand-rolled
semver parser would reject it. ViaVersion's check for the literal
``${version}`` placeholder has no counterpart: hatch-vcs never leaves an
unsubstituted token behind.

See Also:
    com.viaversion.viaversion.update.UpdateUtil
    com.viaversion.viaversion.bukkit.listeners.UpdateListener
"""

from __future__ import annotations

import uuid
from typing import NamedTuple

import aiohttp
from endstone import Logger, Player
from endstone.plugin import Plugin
from packaging.version import InvalidVersion, Version

from ._version import __version__

__all__ = ["send_update_message"]

PREFIX = "§a§l[Endweave] §a"
_RELEASES_URL = "https://api.github.com/repos/EndstoneMC/endweave/releases/latest"


class _UpdateMessage(NamedTuple):
    """A log level paired with the text to report at it."""

    level: Logger.Level
    text: str


def send_update_message(plugin: Plugin, player: Player | None = None) -> None:
    """Check for a newer release and report it, off the server thread.

    Java's two overloads collapse into one call: with no player the result goes
    to the console, otherwise it is sent to that player as a chat message.
    Callers are expected to have checked the ``endweave.update`` permission and
    the ``check-for-updates`` config option first, as ViaVersion's join listener
    does.

    Args:
        plugin: Plugin whose logger, server and scheduler are used.
        player: Recipient of the message, or None to log it to the console.
    """
    import endstone.asyncio

    endstone.asyncio.submit(_send_update_message(plugin, player.unique_id if player is not None else None))


async def _send_update_message(plugin: Plugin, unique_id: uuid.UUID | None) -> None:
    message = await _get_update_message(console=unique_id is None)
    if message is None:
        return

    def deliver() -> None:
        if unique_id is None:
            if message.level is Logger.Level.WARNING:
                plugin.logger.warning(message.text)
            else:
                plugin.logger.info(message.text)
            return
        player = plugin.server.get_player(unique_id)
        if player is not None:
            player.send_message(PREFIX + message.text)

    plugin.server.scheduler.run_task(plugin, deliver)


async def _get_update_message(*, console: bool) -> _UpdateMessage | None:
    try:
        newest = Version(await _get_newest_version())
    except (aiohttp.ClientError, TimeoutError, ValueError, KeyError):
        if console:
            return _UpdateMessage(Logger.Level.WARNING, "Could not check for updates, check your connection.")
        return None

    try:
        current = Version(__version__)
    except InvalidVersion:
        return _UpdateMessage(Logger.Level.INFO, "You are using a custom version, consider updating.")

    if current < newest:
        return _UpdateMessage(
            Logger.Level.WARNING,
            f"There is a newer plugin version available: {newest}, you're on: {current}",
        )
    if console and current != newest:
        if current.is_devrelease:
            return _UpdateMessage(
                Logger.Level.INFO,
                "You are running a development version of the plugin, please report any bugs to GitHub.",
            )
        return _UpdateMessage(Logger.Level.WARNING, "You are running a newer version of the plugin than is released!")
    return None


async def _get_newest_version() -> str:
    headers = {
        "Accept": "application/vnd.github+json",
        "Cache-Control": "no-cache",
        "User-Agent": f"Endweave {__version__}",
    }
    async with aiohttp.ClientSession(timeout=aiohttp.ClientTimeout(total=10)) as session:
        async with session.get(_RELEASES_URL, headers=headers) as response:
            response.raise_for_status()
            release = await response.json()
    tag_name: str = release["tag_name"]
    return tag_name
