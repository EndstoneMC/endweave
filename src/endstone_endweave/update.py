"""Update checker for Endweave plugin.

Checks GitHub releases for newer versions on startup and logs a message
if an update is available. On player join, notifies players with the
``endweave.update`` permission. Notification-only, no auto-download.

See Also:
    com.viaversion.viaversion.update.UpdateUtil
"""

import asyncio
import json
import urllib.request
from typing import Optional

from endstone import Logger, Player
from packaging.version import InvalidVersion, Version

_GITHUB_API_URL = "https://api.github.com/repos/EndstoneMC/endweave/releases/latest"
_PERMISSION = "endweave.update"


class UpdateChecker:
    """Checks for plugin updates and caches the result.

    See Also:
        com.viaversion.viaversion.update.UpdateUtil
    """

    def __init__(self, logger: Logger, current_version: str) -> None:
        self._logger = logger
        self._current_version = current_version
        self._update_message: Optional[str] = None

    def check(self) -> None:
        """Initiate an async update check.

        Submits a coroutine to the endstone asyncio background loop that
        fetches the latest release from GitHub and logs the result.
        """
        import endstone.asyncio

        endstone.asyncio.submit(self._check())

    def notify_if_needed(self, player: Player) -> None:
        """Send the cached update message to a player if they have permission.

        See Also:
            com.viaversion.viaversion.bukkit.listeners.UpdateListener
        """
        if self._update_message and player.has_permission(_PERMISSION):
            player.send_message(self._update_message)

    async def _check(self) -> None:
        newest_string = await _fetch_latest_version()
        if newest_string is None:
            self._logger.warning("Could not check for updates, check your connection.")
            return

        try:
            current = Version(self._current_version)
        except InvalidVersion:
            self._logger.info("You are using a custom version, consider updating.")
            return

        try:
            newest = Version(newest_string)
        except InvalidVersion:
            return

        if current < newest:
            message = f"There is a newer version available: {newest}, you're on: {current}"
            self._logger.warning(message)
            self._update_message = f"[Endweave] {message}"
        elif current > newest:
            if current.is_devrelease or current.is_prerelease:
                self._logger.info("You are running a development version, please report any bugs to GitHub.")
            else:
                self._logger.warning("You are running a newer version than is released!")


async def _fetch_latest_version() -> Optional[str]:
    try:
        return await asyncio.to_thread(_fetch_latest_version_sync)
    except Exception:
        return None


def _fetch_latest_version_sync() -> str:
    request = urllib.request.Request(
        _GITHUB_API_URL,
        headers={
            "Accept": "application/vnd.github.v3+json",
            "User-Agent": "Endweave",
        },
    )
    with urllib.request.urlopen(request, timeout=10) as response:
        data = json.loads(response.read().decode())
    tag_name: str = data["tag_name"]
    return tag_name
