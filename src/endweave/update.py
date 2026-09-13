"""Update notification for Endweave.

Checks the newest published release on startup and on player join, and reports
back through the console or a chat message. Notification only, nothing is
downloaded.

ViaVersion polls its own update server for a plain ``{"name": ...}`` document;
Endweave has no such service and reads the tag of the latest GitHub release
instead. ViaVersion requests it afresh on every check with caching turned off;
here the answer is held for an hour, and a failed check for five minutes, so a
busy server's joins cannot burn through the sixty unauthenticated calls an hour
GitHub allows an address. Version comparison is PEP 440 rather than ViaVersion's
own semver type, so a setuptools-scm ``0.4.4.dev1`` build is recognised where a
hand-rolled semver parser would reject it. ViaVersion's check for the literal
``${version}`` placeholder has no counterpart: setuptools-scm never leaves an
unsubstituted token behind.

See Also:
    com.viaversion.viaversion.update.UpdateUtil
    com.viaversion.viaversion.bukkit.listeners.UpdateListener
"""

from __future__ import annotations

import asyncio
import time
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
_CACHE_TTL = 3600.0
_FAILURE_TTL = 300.0

_cache_lock = asyncio.Lock()
_cached_version: Version | None = None
_cache_expiry = 0.0


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
    newest = await _get_newest_version()
    if newest is None:
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
        if current.is_devrelease or current.is_prerelease:
            return _UpdateMessage(
                Logger.Level.INFO,
                "You are running a development version of the plugin, please report any bugs to GitHub.",
            )
        return _UpdateMessage(Logger.Level.WARNING, "You are running a newer version of the plugin than is released!")
    return None


async def _get_newest_version() -> Version | None:
    """Read the tag of the latest release, through the cache.

    Every caller within the cache window is answered from the last result, and a
    check that failed is only remembered until the shorter retry window is up.

    Returns:
        The newest released version, or None if the check failed.
    """
    global _cached_version, _cache_expiry

    async with _cache_lock:
        now = time.monotonic()
        if now < _cache_expiry:
            return _cached_version

        headers = {
            "Accept": "application/vnd.github+json",
            "Cache-Control": "no-cache",
            "User-Agent": f"Endweave {__version__}",
        }
        try:
            async with aiohttp.ClientSession(timeout=aiohttp.ClientTimeout(total=10)) as session:
                async with session.get(_RELEASES_URL, headers=headers) as response:
                    response.raise_for_status()
                    release = await response.json()
            _cached_version = Version(release["tag_name"])
        except (aiohttp.ClientError, asyncio.TimeoutError, TimeoutError, ValueError, KeyError):
            _cached_version = None
        _cache_expiry = now + (_CACHE_TTL if _cached_version is not None else _FAILURE_TTL)
        return _cached_version
